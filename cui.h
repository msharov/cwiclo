// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#if __has_include(<curses.h>)
#include "app.h"
#include <curses.h>

//--- CursesWindow -----------------------------------------------------
namespace cwiclo {

class CursesWindow : public Msger {
public:
    using key_t	= int;
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
    inline virtual void	draw (void) const { wrefresh (_winput); }
    virtual void	on_key (key_t key);
    void		close (void);
    void		TimerR_timer (PTimerR::fd_t);
protected:
    //{{{ Ctrl ---------------------------------------------------------

    class Ctrl {
    public:
	enum { caret_is_visible = false };
	enum { f_Focused, f_Disabled, f_Last };
	using coord_t	= uint16_t;
	using dim_t	= make_unsigned_t<coord_t>;
	struct Point	{ coord_t x,y; };
	struct Size	{ dim_t	x,y; };
	struct Rect	{ coord_t x,y; dim_t w,h; };
    public:
			Ctrl (void)		:_text(),_w(),_flags() {}
			Ctrl (const Ctrl&) = delete;
			~Ctrl (void)		{ destroy(); }
	auto		w (void) const		{ return _w; }
			operator WINDOW* (void)	const	{ return w(); }
	void		operator= (const Ctrl&) = delete;
	auto		operator-> (void) const	{ return w(); }
	dim_t		maxx (void) const	{ return getmaxx(w()); }
	dim_t		maxy (void) const	{ return getmaxy(w()); }
	coord_t		begx (void) const	{ return getbegx(w()); }
	coord_t		begy (void) const	{ return getbegy(w()); }
	coord_t		endx (void) const	{ return begx()+maxx(); }
	coord_t		endy (void) const	{ return begy()+maxy(); }
	coord_t		curx (void) const	{ return getcurx(w()); }
	coord_t		cury (void) const	{ return getcury(w()); }
	void		draw (void) const	{ werase (w()); }
	void		flush_draw (void) const	{ wnoutrefresh (w()); }
	void		destroy (void)		{ if (auto w = exchange(_w,nullptr); w) delwin(w); }
	void		create (int l, int c, int y, int x);
	auto		flag (unsigned f) const			{ return get_bit(_flags,f); }
	void		set_flag (unsigned f, bool v = true)	{ set_bit(_flags,f,v); }
	auto&		text (void) const			{ return _text; }
	void		set_text (const string_view& t)		{ _text = t; }
	void		set_text (const char* t)		{ _text = t; }
	static Size	measure_text (const string_view& text);
	auto		measure (void) const			{ return measure_text (text()); }
	auto		focused (void) const			{ return flag (f_Focused); }
	void		transfer_focus_to (Ctrl& c)		{ set_flag (f_Focused, false); c.set_flag (f_Focused); }
    protected:
	auto&		textw (void)		{ return _text; }
    private:
	string		_text;
	WINDOW*		_w;
	uint16_t	_flags;
    };
    //}}}---------------------------------------------------------------
    //{{{ Label

    class Label : public Ctrl {
    public:
			Label (void) : Ctrl() {}
	void		draw (void) const;
    };
    //}}}---------------------------------------------------------------
    //{{{ Button

    class Button : public Ctrl {
    public:
	explicit	Button (void)		: Ctrl() {}
	void		create (int c, int y, int x)	{ Ctrl::create (1,c,y,x); }
	uint16_t	measure (void)		{ return measure_text(text()).x+strlen("[  ]"); }
	void		draw (void) const;
    };
    //}}}---------------------------------------------------------------
    //{{{ Listbox

    class Listbox : public Ctrl {
    public:
			Listbox (void)		:Ctrl(),n(),sel(),top() {}
	void		clip_sel (void)		{ if (n && sel > n-1) sel = n-1; }
	void		set_n (dim_t nn)	{ n = nn; clip_sel(); }
	void		on_set_text (void);
	void		set_text (const char* t)	{ Ctrl::set_text(t); on_set_text(); }
	void		set_text (const string_view& t)	{ Ctrl::set_text(t); on_set_text(); }
	void		on_key (key_t k);
	void		draw (void) const;
    public:
	dim_t		n;
	dim_t		sel;
	dim_t		top;
    };
    //}}}---------------------------------------------------------------
    //{{{ Editbox

    class Editbox : public Ctrl {
    public:
	enum { caret_is_visible = true };
    public:
			Editbox (void)			:Ctrl(),_cpos(),_fc() {}
	void		create (int c, int y, int x);
	void		on_set_text (void)		{ _cpos = text().size(); _fc = 0; posclip(); }
	void		set_text (const char* t)	{ Ctrl::set_text(t); on_set_text(); }
	void		set_text (const string_view& t)	{ Ctrl::set_text(t); on_set_text(); }
	void		on_key (key_t k);
	void		draw (void) const;
    private:
	void		posclip (void);
    private:
	coord_t		_cpos;
	u_short		_fc;
    };
    //}}}---------------------------------------------------------------
    //{{{ HSplitter
    class HSplitter : public Ctrl {
    public:
			HSplitter (void)		: Ctrl() {}
	void		create (int c, int y, int x)	{ Ctrl::create (1,c,y,x); }
	void		draw (void) const {
			    Ctrl::draw();
			    whline (w(), 0, maxx());
			    flush_draw();
			}
    };
    //}}}---------------------------------------------------------------
    //{{{ VSplitter
    class VSplitter : public Ctrl {
    public:
			VSplitter (void)		: Ctrl() {}
	void		create (int l, int y, int x)	{ Ctrl::create (l,1,y,x); }
	void		draw (void) const {
			    Ctrl::draw();
			    wvline (w(), 0, maxy());
			    flush_draw();
			}
    };
    //}}}---------------------------------------------------------------
    //{{{ GroupFrame
    class GroupFrame : public Ctrl {
    public:
			GroupFrame (void) : Ctrl() {}
	void		draw (void) const {
			    Ctrl::draw();
			    box (w(), 0, 0);
			    auto tsz = min (text().size(), maxx()-strlen("[  ]"));
			    if (tsz > 0) {
				mvwhline (w(), 0, (maxx()-tsz)/2u-1, ' ', tsz+2);
				mvwaddnstr (w(), 0, (maxx()-tsz)/2u, text().c_str(), tsz);
			    }
			    flush_draw();
			}
    };
    //}}}---------------------------------------------------------------
    //{{{ StatusLine
    class StatusLine : public Ctrl {
    public:
	enum { f_Modified = Ctrl::f_Last, f_Last };
    public:
			StatusLine (void)		: Ctrl() {}
	bool		is_modified (void) const	{ return flag (f_Modified); }
	void		set_modified (bool v = true)	{ set_flag (f_Modified,v); }
	void		create (int c, int y, int x) {
			    Ctrl::create (1,c,y,x);
			    wbkgdset (w(), A_REVERSE);
			}
	void		draw (void) const {
			    Ctrl::draw();
			    waddstr (w(), text().c_str());
			    if (is_modified())
				mvwaddstr (w(), 0, maxx()-strlen(" *"), " *");
			    flush_draw();
			}
    };
    //}}}---------------------------------------------------------------
    //{{{ Focus rotation
private:
    void set_caret_visible (bool v = true)
	{ set_flag (f_CaretOn, v); curs_set(v); }
    template <typename F>
    inline void init_ctrl_focus_iter (F& f) {
	f.set_flag (Ctrl::f_Focused);
	set_input_ctrl (f);
	if (F::caret_is_visible)
	    set_caret_visible();
    }
    template <typename F, typename H, typename... C>
    inline void init_ctrl_focus_iter (F& f, H& h, C&... c)
	{ if (!h.focused()) init_ctrl_focus_iter (f,c...); }
protected:
    template <typename H, typename... C>
    inline void init_ctrl_focus (H& h, C&... c)
	{ init_ctrl_focus_iter (h,h,c...); }
    template <typename F, typename T>
    inline void rotate_ctrl_focus (F& f, T& t) {
	f.transfer_focus_to (t);
	set_input_ctrl (t);
	if (F::caret_is_visible != T::caret_is_visible)
	    set_caret_visible (T::caret_is_visible);
    }
    //}}}---------------------------------------------------------------
protected:
    void		set_input_ctrl (Ctrl& c);
    inline virtual void	layout (void) { draw(); }
private:
    WINDOW*		_winput;
    PTimer		_uiinput;
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
	auto prompt = string_view_from_const_stream(is);
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
    void		draw (void) const override;
    void		on_key (key_t key) override;
private:
    void		done (Answer answer);
private:
    string		_prompt;
    Label		_wmsg;
    Button		_wcancel;
    Button		_wok;
    Button		_wignore;
    GroupFrame		_wframe;
    Type		_type;
    Answer		_answer;
    PMessageBoxR	_reply;
};

} // namespace cwiclo
#endif // __has_include(<curses.h>)
