// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#if __has_include(<curses.h>)
#include "widget.h"
#include "app.h"

namespace cwiclo {
namespace ui {

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
    using widgetvec_t	= vector<unique_ptr<Widget>>;
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
    virtual void	layout (void);
    Widget*		create_widget (const Widget::Layout& l);
    void		create_widgets (const Widget::Layout* l, unsigned sz);
    template <unsigned N>
    void		create_widgets (const Widget::Layout (&l)[N])
			    { create_widgets (begin(l), size(l)); }
    void		destroy_widgets (void)	{ _widgets.clear(); }
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
    Rect		compute_size_hints (const Widget::Layout& plinfo, widgetvec_t::iterator& f, widgetvec_t::iterator l);
    widgetvec_t::iterator layout_widget (const Rect& area, widgetvec_t::iterator f, widgetvec_t::iterator l);
private:
    WINDOW*		_winput;
    PTimer		_uiinput;
    widgetid_t		_focused;
    widgetvec_t		_widgets;
    static mrid_t	s_focused;
    static uint8_t	s_nwins;
};

} // namespace ui
} // namespace cwiclo
#endif // __has_include(<curses.h>)
