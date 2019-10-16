// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#if __has_include(<curses.h>)
#include "app.h"
#include <curses.h>

//--- Event ------------------------------------------------------------
namespace cwiclo {

class Event {
public:
    using widgetid_t	= uint16_t;
    using key_t		= uint32_t;
    using coord_t	= int16_t;
    using dim_t		= make_unsigned_t<coord_t>;
    struct Point { coord_t x,y; };
    struct Size	{ dim_t	w,h; };
    struct Rect	{ coord_t x,y; dim_t w,h; };
    enum class Type : uint8_t {
	None,
	KeyDown,
	Selection
    };
    enum : widgetid_t {
	wid_None,
	wid_First,
	wid_Last = numeric_limits<widgetid_t>::max()
    };
public:
    constexpr		Event (void)
			    :_src(wid_None),_type(Type::None),_mods(0),_key(0) {}
    constexpr		Event (Type t, key_t k, widgetid_t src = wid_None)
			    :_src(src),_type(t),_mods(0),_key(k) {}
    constexpr		Event (Type t, const Point& pt, uint8_t mods = 0, widgetid_t src = wid_None)
			    :_src(src),_type(t),_mods(mods),_pt(pt) {}
    constexpr		Event (Type t, const Size& sz, uint8_t mods = 0, widgetid_t src = wid_None)
			    :_src(src),_type(t),_mods(mods),_sz(sz) {}
    constexpr auto	src (void) const	{ return _src; }
    constexpr auto	type (void) const	{ return _type; }
    constexpr auto&	loc (void) const	{ return _pt; }
    constexpr auto	selection_start (void) const	{ return _sz.w; }
    constexpr auto	selection_end (void) const	{ return _sz.h; }
    constexpr auto	key (void) const	{ return _key; }
    constexpr void	read (istream& is)	{ is.readt (*this); }
    template <typename Stm>
    constexpr void	write (Stm& os) const	{ os.writet (*this); }
private:
    widgetid_t	_src;
    Type	_type;
    uint8_t	_mods;
    union {
	Point	_pt;
	Size	_sz;
	key_t	_key;
    };
};
#define SIGNATURE_Event	"(qyyu)"

//--- Widget -----------------------------------------------------------

class PWidgetR : public ProxyR {
    DECLARE_INTERFACE (WidgetR, (event,SIGNATURE_Event)(modified,"qqs"))
public:
    using widgetid_t	= Event::widgetid_t;
public:
    constexpr		PWidgetR (const Msg::Link& l)	: ProxyR(l) {}
    void		event (const Event& ev)		{ send (m_event(), ev); }
    void		modified (widgetid_t wid, const string& t)
			    { send (m_modified(), wid, uint16_t(0), t); }
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() == m_event())
	    o->PWidgetR_event (msg.read().read<Event>());
	else if (msg.method() == m_modified()) {
	    auto is = msg.read();
	    auto wid = is.read<widgetid_t>();
	    is.skip (2);
	    o->PWidgetR_modified (wid, msg.read().read<string_view>());
	} else
	    return false;
	return true;
    }
};

//--- CursesWindow -----------------------------------------------------

class CursesWindow : public Msger {
public:
    using key_t		= Event::key_t;
    using widgetid_t	= Event::widgetid_t;
    enum : widgetid_t {
	wid_None,
	wid_First,
	wid_Last = numeric_limits<widgetid_t>::max()
    };
    using coord_t	= Event::coord_t;
    using dim_t		= Event::dim_t;
    using Point		= Event::Point;
    using Size		= Event::Size;
    using Rect		= Event::Rect;
    enum { COLOR_DEFAULT = -1 };	// Curses transparent color
    enum {			// Keys that curses does not define
	KEY_ESCAPE = '\e',
	KEY_BKSPACE = '~'+1
    };
    enum { f_CaretOn = Msger::f_Last, f_Last };
public:
    explicit		CursesWindow (const Msg::Link& l);
			~CursesWindow (void) override;
    bool		dispatch (Msg& msg) override;
    void		on_msger_destroyed (mrid_t mid) override;
    virtual void	draw (void) const;
    virtual void	on_event (const Event& ev);
    virtual void	on_modified (widgetid_t, const string_view&) {}
    virtual void	on_key (key_t key);
    void		close (void);
    void		TimerR_timer (PTimerR::fd_t);
    void		PWidget_event (const Event& ev)		{ on_event (ev); }
    void		PWidgetR_event (const Event& ev)	{ on_event (ev); }
    void		PWidgetR_modified (widgetid_t wid, const string_view& t)
			    { on_modified (wid, t); }
protected:
    //{{{ Widget ---------------------------------------------------------

    class Widget {
    public:
	enum { f_Focused, f_CanFocus, f_HasCaret, f_Disabled, f_Modified, f_Last };
	enum class Type : uint8_t {
	    Widget,
	    Label,
	    Button,
	    Listbox,
	    Editbox,
	    HSplitter,
	    VSplitter,
	    GroupFrame,
	    StatusLine
	};
	enum class HAlign : uint8_t { Left, Center, Right };
	enum class VAlign : uint8_t { Top, Center, Bottom };
	struct Layout {
	    uint8_t	level	= 0;
	    Type	type	= Type::Widget;
	    widgetid_t	id	= 0;
	    HAlign	halign	= HAlign::Left;
	    VAlign	valign	= VAlign::Top;
	};
	#define WL_(tn,...)	{ .type=Widget::Type::tn, ## __VA_ARGS__ }
	#define WL___(tn,...)	{ .level=1, .type=Widget::Type::tn, ## __VA_ARGS__ }
	#define WL_____(tn,...)	{ .level=2, .type=Widget::Type::tn, ## __VA_ARGS__ }
	#define WL_______(tn,...)	{ .level=3, .type=Widget::Type::tn, ## __VA_ARGS__ }
	#define WL_________(tn,...)	{ .level=4, .type=Widget::Type::tn, ## __VA_ARGS__ }
	#define WL___________(tn,...)	{ .level=5, .type=Widget::Type::tn, ## __VA_ARGS__ }
	#define WL_____________(tn,...)	{ .level=6, .type=Widget::Type::tn, ## __VA_ARGS__ }
	#define WL_______________(tn,...)	{ .level=7, .type=Widget::Type::tn, ## __VA_ARGS__ }
	#define WL_________________(tn,...)	{ .level=8, .type=Widget::Type::tn, ## __VA_ARGS__ }
    public:
	explicit	Widget (const Msg::Link& l, const Layout& lay);
			Widget (const Widget&) = delete;
	void		operator= (const Widget&) = delete;
	virtual		~Widget (void)		{ destroy(); }
	auto		w (void) const		{ return _w; }
	auto&		layinfo (void) const	{ return _layinfo; }
	auto		widget_id (void) const	{ return _layinfo.id; }
			operator WINDOW* (void)	const	{ return w(); }
	auto		operator-> (void) const	{ return w(); }
	inline dim_t	maxx (void) const	{ return getmaxx(w()); }
	inline dim_t	maxy (void) const	{ return getmaxy(w()); }
	inline coord_t	begx (void) const	{ return getbegx(w()); }
	inline coord_t	begy (void) const	{ return getbegy(w()); }
	inline coord_t	endx (void) const	{ return begx()+maxx(); }
	inline coord_t	endy (void) const	{ return begy()+maxy(); }
	inline coord_t	curx (void) const	{ return getcurx(w()); }
	inline coord_t	cury (void) const	{ return getcury(w()); }
	auto&		size_hints (void) const			{ return _size_hints; }
	void		set_size_hints (const Size& sh)		{ _size_hints = sh; }
	void		set_size_hints (dim_t w, dim_t h)	{ _size_hints.w = w; _size_hints.h = h; }
	void		set_selection (const Size& s)		{ _selection = s; }
	void		set_selection (dim_t f, dim_t t)	{ _selection.w = f; _selection.h = t; }
	void		set_selection (dim_t f)			{ _selection.w = f; _selection.h = f+1; }
	auto&		selection (void) const			{ return _selection; }
	auto		selection_start (void) const		{ return selection().w; }
	auto		selection_end (void) const		{ return selection().w; }
	auto&		area (void) const			{ return _area; }
	void		set_area (const Rect& r)		{ _area = r; }
	void		set_area (coord_t x, coord_t y, dim_t w, dim_t h)	{ _area.x = x; _area.y = y; _area.w = w; _area.h = h; }
	void		set_area (const Point& p, const Size& sz)		{ _area.x = p.x; _area.y = p.y; _area.w = sz.w; _area.h = sz.h; }
	void		set_area (const Point& p)		{ _area.x = p.x; _area.y = p.y; }
	void		set_area (const Size& sz)		{ _area.w = sz.w; _area.h = sz.h; }
	void		draw (void) const			{ on_draw(); wnoutrefresh (w()); }
	virtual void	on_draw (void) const			{ werase (w()); }
	void		destroy (void)				{ if (auto w = exchange(_w,nullptr); w) delwin(w); }
	virtual void	resize (int l, int c, int y, int x);
	auto		flag (unsigned f) const			{ return get_bit(_flags,f); }
	void		set_flag (unsigned f, bool v = true)	{ set_bit(_flags,f,v); }
	bool		is_modified (void) const		{ return flag (f_Modified); }
	void		set_modified (bool v = true)		{ set_flag (f_Modified,v); }
	auto&		text (void) const			{ return _text; }
	void		set_text (const string_view& t)		{ _text = t; on_set_text(); }
	void		set_text (const char* t)		{ _text = t; on_set_text(); }
	virtual void	on_set_text (void);
	virtual void	on_resize (void)			{ }
	virtual void	on_event (const Event& ev);
	virtual void	on_key (key_t);
	static Size	measure_text (const string_view& text);
	auto		measure (void) const			{ return measure_text (text()); }
	auto		focused (void) const			{ return flag (f_Focused); }
    protected:
	auto&		textw (void)				{ return _text; }
	void		create_event (const Event& ev)		{ _reply.event (ev); }
	void		report_modified (void);
	void		report_selection (void)
			    { create_event (Event (Event::Type::Selection, _selection, 0, widget_id())); }
    private:
	string		_text;
	WINDOW*		_w;
	Rect		_area;
	Size		_size_hints;
	Size		_selection;
	uint16_t	_flags;
	Layout		_layinfo;
	PWidgetR	_reply;
    };

    using widgetvec_t	= vector<unique_ptr<Widget>>;
    //}}}---------------------------------------------------------------
    //{{{ Label

    class Label : public Widget {
    public:
	explicit	Label (const Msg::Link& l, const Layout& lay) : Widget(l,lay) {}
	void		on_draw (void) const override;
    };
    //}}}---------------------------------------------------------------
    //{{{ Button

    class Button : public Widget {
    public:
	explicit	Button (const Msg::Link& l, const Layout& lay)
			    : Widget(l,lay) { set_flag (f_CanFocus); }
	void		on_draw (void) const override;
	void		on_set_text (void) override;
    };
    //}}}---------------------------------------------------------------
    //{{{ Listbox

    class Listbox : public Widget {
    public:
	explicit	Listbox (const Msg::Link& l, const Layout& lay)
			    : Widget(l,lay),_n(),_top() { set_flag (f_CanFocus); }
	void		on_set_text (void) override;
	void		on_key (key_t k) override;
	void		on_draw (void) const override;
    private:
	void		clip_sel (void)	{ set_selection (min (selection_start(), _n-1)); }
	void		set_n (dim_t n)	{ _n = n; clip_sel(); }
    private:
	dim_t		_n;
	dim_t		_top;
    };
    //}}}---------------------------------------------------------------
    //{{{ Editbox

    class Editbox : public Widget {
    public:
	explicit	Editbox (const Msg::Link& l, const Layout& lay)
			    : Widget(l,lay),_cpos(),_fc() { set_flag (f_HasCaret); set_flag (f_CanFocus); }
	void		on_resize (void) override;
	void		on_set_text (void) override;
	void		on_key (key_t k) override;
	void		on_draw (void) const override;
    private:
	void		posclip (void);
    private:
	coord_t		_cpos;
	u_short		_fc;
    };
    //}}}---------------------------------------------------------------
    //{{{ HSplitter
    class HSplitter : public Widget {
    public:
	explicit	HSplitter (const Msg::Link& l, const Layout& lay)
			    : Widget(l,lay) {}
	void		on_draw (void) const override {
			    Widget::on_draw();
			    whline (w(), 0, maxx());
			}
    };
    //}}}---------------------------------------------------------------
    //{{{ VSplitter
    class VSplitter : public Widget {
    public:
	explicit	VSplitter (const Msg::Link& l, const Layout& lay)
			    : Widget(l,lay) {}
	void		on_draw (void) const override {
			    Widget::on_draw();
			    wvline (w(), 0, maxy());
			}
    };
    //}}}---------------------------------------------------------------
    //{{{ GroupFrame
    class GroupFrame : public Widget {
    public:
	explicit	GroupFrame (const Msg::Link& l, const Layout& lay)
			    : Widget(l,lay) {}
	void		on_draw (void) const override {
			    Widget::on_draw();
			    box (w(), 0, 0);
			    auto tsz = min (text().size(), maxx()-strlen("[  ]"));
			    if (tsz > 0) {
				mvwhline (w(), 0, (maxx()-tsz)/2u-1, ' ', tsz+2);
				mvwaddnstr (w(), 0, (maxx()-tsz)/2u, text().c_str(), tsz);
			    }
			}
    };
    //}}}---------------------------------------------------------------
    //{{{ StatusLine
    class StatusLine : public Widget {
    public:
	enum { f_Modified = Widget::f_Last, f_Last };
    public:
	explicit	StatusLine (const Msg::Link& l, const Layout& lay)
			    : Widget(l,lay) {}
	void		on_resize (void) override {
			    Widget::on_resize();
			    wbkgdset (w(), A_REVERSE);
			}
	void		on_draw (void) const override {
			    Widget::on_draw();
			    waddstr (w(), text().c_str());
			    if (is_modified())
				mvwaddstr (w(), 0, maxx()-strlen(" *"), " *");
			}
    };
    //}}}---------------------------------------------------------------
protected:
    virtual void	layout (void);
    Widget*		create_widget (const Widget::Layout& l);
    void		create_widgets (const Widget::Layout* l, unsigned sz);
    template <unsigned N>
    void		create_widgets (const Widget::Layout (&l)[N])
			    { create_widgets (begin(l), size(l)); }
    const Widget*	widget_by_id (widgetid_t id) const PURE;
    Widget*		widget_by_id (widgetid_t id)
			    { return UNCONST_MEMBER_FN (widget_by_id,id); }
    void		set_widget_text (widgetid_t id, const char* t)
			    { if (auto w = widget_by_id (id); w) w->set_text (t); }
    void		set_widget_text (widgetid_t id, const string& t)
			    { if (auto w = widget_by_id (id); w) w->set_text (t); }
    void		set_widget_selection (widgetid_t id, const Size& s)
			    { if (auto w = widget_by_id (id); w) w->set_selection (s); }
    void		set_widget_selection (widgetid_t id, dim_t f, dim_t t)
			    { if (auto w = widget_by_id (id); w) w->set_selection (f,t); }
    void		set_widget_selection (widgetid_t id, dim_t f)
			    { if (auto w = widget_by_id (id); w) w->set_selection (f); }
    void		focus_widget (widgetid_t id);
    auto		focused_widget_id (void) const	{ return _focused; }
    const Widget*	focused_widget (void) const
			    { return widget_by_id (focused_widget_id()); }
    Widget*		focused_widget (void)
			    { return UNCONST_MEMBER_FN (focused_widget); }
    void		focus_next (void);
    void		focus_prev (void);
private:
    WINDOW*		_winput;
    PTimer		_uiinput;
    widgetid_t		_focused;
    widgetvec_t		_widgets;
    static mrid_t	s_focused;
    static uint8_t	s_nwins;
};

//--- MessageBox -------------------------------------------------------

//{{{ PMessageBox ------------------------------------------------------

class PMessageBox : public Proxy {
    DECLARE_INTERFACE (MessageBox, (ask,"qqs"))
public:
    enum class Answer : uint16_t { Cancel, Ok, Ignore, Yes = Ok, Retry = Ok, No = Ignore };
    enum class Type : uint16_t { Ok, OkCancel, YesNo, YesNoCancel, RetryCancelIgnore };
public:
    constexpr explicit	PMessageBox (mrid_t caller) : Proxy (caller) {}
    void		ask (const string& prompt, Type type = Type())
			    { send (m_ask(), type, uint16_t(0), prompt); }
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() != m_ask())
	    return false;
	auto is = msg.read();
	auto type = is.read<Type>();
	auto flags = is.read<uint16_t>();
	auto prompt = is.read<string_view>();
	o->MessageBox_ask (prompt, type, flags);
	return true;
    }
};

//}}}-------------------------------------------------------------------
//{{{ PMessageBoxR

class PMessageBoxR : public ProxyR {
    DECLARE_INTERFACE (MessageBoxR, (answer,"q"))
public:
    using Answer = PMessageBox::Answer;
public:
    explicit		PMessageBoxR (const Msg::Link& l) : ProxyR (l) {}
    void		reply (Answer answer) { send (m_answer(), answer); }
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() != m_answer())
	    return false;
	o->MessageBoxR_reply (msg.read().read<Answer>());
	return true;
    }
};

//}}}-------------------------------------------------------------------

class MessageBox : public CursesWindow {
    using Type = PMessageBox::Type;
    using Answer = PMessageBox::Answer;
public:
    explicit		MessageBox (const Msg::Link& l);
    bool		dispatch (Msg& msg) override;
    inline void		MessageBox_ask (const string_view& prompt, Type type, uint16_t flags);
    void		layout (void) override;
    void		on_key (key_t key) override;
private:
    void		done (Answer answer);
private:
    string		_prompt;
    Type		_type;
    Answer		_answer;
    PMessageBoxR	_reply;
    enum {
	wid_Frame = wid_First,
	wid_Message,
	wid_Cancel,
	wid_OK,
	wid_Ignore
    };
    static constexpr const Widget::Layout c_layout[] = {
	WL_(GroupFrame,	.id=wid_Frame),
	WL_(Label,	.id=wid_Message),
	WL_(Button,	.id=wid_Cancel),
	WL_(Button,	.id=wid_OK),
	WL_(Button,	.id=wid_Ignore)
    };
};

//----------------------------------------------------------------------

} // namespace cwiclo
#endif // __has_include(<curses.h>)
