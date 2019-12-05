// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#if __has_include(<curses.h>)
#include "widget.h"
#include "termrend.h"
#include "app.h"

namespace cwiclo {
namespace ui {

class Window : public Msger {
public:
    using key_t		= Event::key_t;
    using Layout	= Widget::Layout;
    using drawlist_t	= Widget::drawlist_t;
    enum { f_CaretOn = Msger::f_Last, f_Last };
    enum { COLOR_DEFAULT = -1 };	// Curses transparent color
    enum {			// Keys that curses does not define
	KEY_ESCAPE = '\e',
	KEY_BKSPACE = '~'+1
    };
public:
    explicit		Window (const Msg::Link& l);
			~Window (void) override;
    bool		dispatch (Msg& msg) override;
    void		on_msger_destroyed (mrid_t mid) override;
    virtual void	draw (void);
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
    void		create_widgets (const Widget::Layout* f, const Widget::Layout* l);
    template <unsigned N>
    void		create_widgets (const Widget::Layout (&l)[N])
			    { create_widgets (begin(l), end(l)); }
    void		destroy_widgets (void)			{ _widgets.reset(); }
    auto		widget_by_id (widgetid_t id) const	{ return _widgets->widget_by_id(id); }
    auto		widget_by_id (widgetid_t id)		{ return _widgets->widget_by_id(id); }
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
    auto&		w (void)	{ return _w; }
    auto&		w (void) const	{ return _w; }
private:
    TerminalWindowRenderer	_w;
    PTimer		_uiinput;
    widgetid_t		_focused;
    unique_ptr<Widget>	_widgets;
    static mrid_t	s_focused;
    static uint8_t	s_nwins;
};

} // namespace ui
} // namespace cwiclo
#endif // __has_include(<curses.h>)
