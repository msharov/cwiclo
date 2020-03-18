// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "draw.h"

namespace cwiclo {
namespace ui {

//{{{ PWidgetR ---------------------------------------------------------

class PWidgetR : public ProxyR {
    DECLARE_INTERFACE (WidgetR, (event,SIGNATURE_ui_Event)(modified,"qqs")(selection,SIGNATURE_ui_Size"q"))
public:
    constexpr		PWidgetR (const Msg::Link& l)	: ProxyR(l) {}
    constexpr		PWidgetR (mrid_t f, mrid_t t)	: ProxyR(Msg::Link{f,t}) {}
    void		modified (widgetid_t wid, const string& t) const
			    { resend (m_modified(), wid, uint16_t(0), t); }
    void		selection (widgetid_t wid, const Size& s) const
			    { resend (m_selection(), s, wid); }
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() == m_modified()) {
	    auto is = msg.read();
	    auto wid = is.read<widgetid_t>();
	    is.skip (2);
	    o->PWidgetR_modified (wid, is.read<string_view>());
	} else if (msg.method() == m_selection()) {
	    auto is = msg.read();
	    decltype(auto) s = is.read<Size>();
	    o->PWidgetR_selection (is.read<widgetid_t>(), move(s));
	} else
	    return false;
	return true;
    }
};

//}}}
//--- Widget ---------------------------------------------------------

class Window;

class Widget {
public:
    using key_t		= Event::key_t;
    using Layout	= WidgetLayout;
    using Type		= Layout::Type;
    using drawlist_t	= PScreen::drawlist_t;
    using widgetvec_t	= vector<unique_ptr<Widget>>;
    using widget_factory_t	= Widget* (*)(Window* w, const Layout& l);
    enum { f_Focused, f_CanFocus, f_Disabled, f_Modified, f_ForcedSizeHints, f_Last };
    struct alignas(2) BytePoint { uint8_t x,y; };
private:
    struct FocusNeighbors { widgetid_t first, prev, next, last; };
    void		get_focus_neighbors_for (widgetid_t w, FocusNeighbors& n) const;
    FocusNeighbors	get_focus_neighbors_for (widgetid_t w) const;
public:
    explicit		Widget (Window* w, const Layout& lay);
			Widget (const Widget&) = delete;
    void		operator= (const Widget&) = delete;
    virtual		~Widget (void) {}
    [[nodiscard]] static Widget* create (Window* w, const Layout& l) { return s_factory (w, l); }
    [[nodiscard]] static Widget* default_factory (Window* w, const Layout& l);	// body in cwidgets.cc
    static void		set_factory (widget_factory_t f)	{ s_factory = f; }
    auto&		layinfo (void) const			{ return _layinfo; }
    auto		widget_id (void) const			{ return layinfo().id(); }
    auto&		size_hints (void) const			{ return _size_hints; }
    auto&		expandables (void) const		{ return _nexp; }
    void		set_size_hints (const Size& sh)		{ _size_hints = sh; set_flag (f_ForcedSizeHints); }
    void		set_size_hints (dim_t w, dim_t h)	{ set_size_hints (Size(w,h)); }
    void		set_selection (const Size& s)		{ _selection = s; }
    void		set_selection (dim_t f, dim_t t)	{ set_selection (Size(f,t)); }
    void		set_selection (dim_t f)			{ set_selection (f,f+1); }
    auto&		selection (void) const			{ return _selection; }
    auto		selection_start (void) const		{ return selection().w; }
    auto		selection_end (void) const		{ return selection().h; }
    const Widget*	widget_by_id (widgetid_t id) const PURE;
    Widget*		widget_by_id (widgetid_t id)		{ return UNCONST_MEMBER_FN (widget_by_id,id); }
    const Layout*	add_widgets (const Layout* f, const Layout* l);
    template <size_t N>
    auto		add_widgets (const Layout (&l)[N])	{ return add (begin(l), end(l)); }
    Widget*		replace_widget (unique_ptr<Widget>&& nw);
    void		delete_widgets (void)			{ _widgets.clear(); }
    auto&		area (void) const			{ return _area; }
    void		set_area (const Rect& r)		{ _area = r; }
    void		set_area (coord_t x, coord_t y, dim_t w, dim_t h)	{ _area.x = x; _area.y = y; _area.w = w; _area.h = h; }
    void		set_area (const Point& p, const Size& sz)		{ _area.assign (p,sz); }
    void		set_area (const Point& p)		{ _area.move_to (p); }
    void		set_area (const Size& sz)		{ _area.resize (sz); }
    void		draw (drawlist_t& dl) const;
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
    virtual void	on_set_text (void)			{ }
    virtual void	on_resize (void)			{ }
    virtual void	on_event (const Event& ev);
    virtual void	on_key (key_t);
    static Size		measure_text (const string_view& text);
    auto		measure (void) const			{ return measure_text (text()); }
    auto		focused (void) const			{ return flag (f_Focused); }
    Rect		compute_size_hints (void);
    void		resize (const Rect& area);
protected:
    auto		parent_window (void) const		{ return _win; }
    auto&		textw (void)				{ return _text; }
    void		report_modified (void) const;
    void		report_selection (void) const;
private:
    inline PWidgetR	widget_reply (void) const;
    virtual void	on_draw (drawlist_t&) const {}
private:
    string		_text;
    widgetvec_t		_widgets;
    Window*		_win;
    Rect		_area;
    Size		_size_hints;
    Size		_selection;
    uint16_t		_flags;
    BytePoint		_nexp;
    Layout		_layinfo;
    static widget_factory_t s_factory;
};

//{{{ Drawlist writer templates ----------------------------------------

// Drawlist writer templates
#define DEFINE_WIDGET_WRITE_DRAWLIST(widget, dltype, dlw)\
	void widget::on_draw (drawlist_t& dl) const {	\
	    dltype::Writer<sstream> dlss;		\
	    dlss.viewport (area());			\
	    write_drawlist (dlss);			\
	    dl.reserve (dl.size()+dlss.size());		\
	    dltype::Writer<ostream> dlos (ostream (dl.end(), dlss.size()));\
	    dlos.viewport (area());			\
	    write_drawlist (dlos);			\
	    assert (dlss.size() == dltype::validate (istream (dl.end(), dlss.size()))\
		    && "Drawlist template size incorrectly computed");\
	    dl.shrink (dl.size()+dlss.size());		\
	}						\
	template <typename S>				\
	void widget::write_drawlist (dltype::Writer<S>& dlw) const
#define DECLARE_WIDGET_WRITE_DRAWLIST(dltype)		\
	void on_draw (drawlist_t& dl) const override;	\
	template <typename S>				\
	void write_drawlist (dltype::Writer<S>& drw) const

//}}}-------------------------------------------------------------------
//{{{ Widget factory

#define DECLARE_WIDGET_FACTORY(name)\
    static ::cwiclo::ui::Widget* name (::cwiclo::ui::Window* w, const ::cwiclo::ui::Widget::Layout& lay);\

#define BEGIN_WIDGET_FACTORY(name)\
    ::cwiclo::ui::Widget* name (::cwiclo::ui::Window* w, const ::cwiclo::ui::Widget::Layout& lay) {\
	switch (lay.type()) {\
	    default: return new ::cwiclo::ui::Widget (w,lay);
#define WIDGET_TYPE_IMPLEMENT(type,impl) \
	    case ::cwiclo::ui::Widget::Type::type: return new impl (w,lay);
#define END_WIDGET_FACTORY }}

#define SET_WIDGET_FACTORY(name)\
    namespace cwiclo { namespace ui { Widget::widget_factory_t Widget::s_factory = name; }}

//}}}-------------------------------------------------------------------

} // namespace ui
} // namespace cwiclo
