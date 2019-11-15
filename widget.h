// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#if __has_include(<curses.h>)
#include "event.h"
#include <curses.h>

namespace cwiclo {
namespace ui {

//{{{ PWidgetR ---------------------------------------------------------

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
	    o->PWidgetR_modified (wid, is.read<string_view>());
	} else
	    return false;
	return true;
    }
};

//}}}
//--- Widget ---------------------------------------------------------

class Widget {
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
    using widgetvec_t	= vector<unique_ptr<Widget>>;
    enum { COLOR_DEFAULT = -1 };	// Curses transparent color
    enum {			// Keys that curses does not define
	KEY_ESCAPE = '\e',
	KEY_BKSPACE = '~'+1
    };
    enum { f_Focused, f_CanFocus, f_HasCaret, f_Disabled, f_Modified, f_Last };
    enum class Type : uint8_t {
	Widget,
	HBox,
	VBox,
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
    using widget_factory_t	= Widget* (*)(mrid_t owner, const Layout& l);
    //{{{ WL_ macros ----------------------------------------------------
    #define WL_L(l,tn,...)	{ .level=l, .type=::cwiclo::ui::Widget::Type::tn, ## __VA_ARGS__ }
    #define WL_(tn,...)					WL_L( 1,tn, ## __VA_ARGS__)
    #define WL___(tn,...)				WL_L( 2,tn, ## __VA_ARGS__)
    #define WL_____(tn,...)				WL_L( 3,tn, ## __VA_ARGS__)
    #define WL_______(tn,...)				WL_L( 4,tn, ## __VA_ARGS__)
    #define WL_________(tn,...)				WL_L( 5,tn, ## __VA_ARGS__)
    #define WL___________(tn,...)			WL_L( 6,tn, ## __VA_ARGS__)
    #define WL_____________(tn,...)			WL_L( 7,tn, ## __VA_ARGS__)
    #define WL_______________(tn,...)			WL_L( 8,tn, ## __VA_ARGS__)
    #define WL_________________(tn,...)			WL_L( 9,tn, ## __VA_ARGS__)
    #define WL___________________(tn,...)		WL_L(10,tn, ## __VA_ARGS__)
    #define WL_____________________(tn,...)		WL_L(11,tn, ## __VA_ARGS__)
    #define WL_______________________(tn,...)		WL_L(12,tn, ## __VA_ARGS__)
    #define WL_________________________(tn,...)		WL_L(13,tn, ## __VA_ARGS__)
    #define WL___________________________(tn,...)	WL_L(14,tn, ## __VA_ARGS__)
    #define WL_____________________________(tn,...)	WL_L(15,tn, ## __VA_ARGS__)
    //}}}----------------------------------------------------------------
private:
    struct FocusNeighbors { widgetid_t first, prev, next, last; };
    void		get_focus_neighbors_for (widgetid_t w, FocusNeighbors& n) const;
    FocusNeighbors	get_focus_neighbors_for (widgetid_t w) const;
public:
    explicit		Widget (const Msg::Link& l, const Layout& lay);
			Widget (const Widget&) = delete;
    void		operator= (const Widget&) = delete;
    virtual		~Widget (void)			{ destroy(); }
    [[nodiscard]] static Widget*	create (mrid_t owner, const Layout& l)	{ return s_factory (owner, l); }
    auto		w (void) const			{ return _w; }
    auto&		layinfo (void) const		{ return _layinfo; }
    auto		widget_id (void) const		{ return _layinfo.id; }
			operator WINDOW* (void)	const	{ return w(); }
    auto		operator-> (void) const		{ return w(); }
    auto&		size_hints (void) const			{ return _size_hints; }
    void		set_size_hints (const Size& sh)		{ _size_hints = sh; }
    void		set_size_hints (dim_t w, dim_t h)	{ _size_hints.w = w; _size_hints.h = h; }
    void		set_selection (const Size& s)		{ _selection = s; }
    void		set_selection (dim_t f, dim_t t)	{ _selection.w = f; _selection.h = t; }
    void		set_selection (dim_t f)			{ _selection.w = f; _selection.h = f+1; }
    auto&		selection (void) const			{ return _selection; }
    auto		selection_start (void) const		{ return selection().w; }
    auto		selection_end (void) const		{ return selection().w; }
    const Widget*	widget_by_id (widgetid_t id) const PURE;
    Widget*		widget_by_id (widgetid_t id)		{ return UNCONST_MEMBER_FN (widget_by_id,id); }
    const Layout*	add_widgets (const Layout* f, const Layout* l);
    template <size_t N>
    auto		add_widgets (const Layout (&l)[N])	{ return add (begin(l), end(l)); }
    void		delete_widgets (void)			{ _widgets.clear(); }
    auto&		area (void) const			{ return _area; }
    void		set_area (const Rect& r)		{ _area = r; }
    void		set_area (coord_t x, coord_t y, dim_t w, dim_t h)	{ _area.x = x; _area.y = y; _area.w = w; _area.h = h; }
    void		set_area (const Point& p, const Size& sz)		{ _area.xy = p; _area.wh = sz; }
    void		set_area (const Point& p)		{ _area.xy = p; }
    void		set_area (const Size& sz)		{ _area.wh = sz; }
    void		draw (void) const;
    virtual void	on_draw (void) const			{ werase (w()); }
    void		destroy (void)				{ if (auto w = exchange(_w,nullptr); w) delwin(w); }
    void		resize (int l, int c, int y, int x);
    void		resize (const Rect& r)			{ resize (r.h, r.w, r.y, r.x); }
    auto		flag (unsigned f) const			{ return get_bit(_flags,f); }
    void		set_flag (unsigned f, bool v = true)	{ set_bit(_flags,f,v); }
    bool		is_modified (void) const		{ return flag (f_Modified); }
    void		set_modified (bool v = true)		{ set_flag (f_Modified,v); }
    auto&		text (void) const			{ return _text; }
    void		set_text (const string_view& t)		{ _text = t; on_set_text(); }
    void		set_text (const char* t)		{ _text = t; on_set_text(); }
    void		focus (widgetid_t id);
    auto		next_focus (widgetid_t wid) const	{ return get_focus_neighbors_for (wid).next; }
    auto		prev_focus (widgetid_t wid) const	{ return get_focus_neighbors_for (wid).prev; }
    virtual void	on_set_text (void);
    virtual void	on_resize (void)			{ }
    virtual void	on_event (const Event& ev);
    virtual void	on_key (key_t);
    static Size		measure_text (const string_view& text);
    auto		measure (void) const			{ return measure_text (text()); }
    auto		focused (void) const			{ return flag (f_Focused); }
    Rect		compute_size_hints (void);
    void		place (const Rect& area);
protected:
    auto&		textw (void)				{ return _text; }
    void		create_event (const Event& ev)		{ _reply.event (ev); }
    void		report_modified (void);
    void		report_selection (void)
			    { create_event (Event (Event::Type::Selection, _selection, 0, widget_id())); }
private:
    string		_text;
    widgetvec_t		_widgets;
    WINDOW*		_w;
    Rect		_area;
    Size		_size_hints;
    Size		_selection;
    uint16_t		_flags;
    Layout		_layinfo;
    PWidgetR		_reply;
    // Declared here, set by default to one in cwidgets.cc, creating all core widgets
    static widget_factory_t	s_factory;
};

} // namespace ui
} // namespace cwiclo
#endif // __has_include(<curses.h>)
