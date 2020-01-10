// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "widget.h"

namespace cwiclo {
namespace ui {

class Widget;

class Window : public Msger {
public:
    using key_t		= Event::key_t;
    using Layout	= WidgetLayout;
    using Info		= WindowInfo;
    using drawlist_t	= PScreen::drawlist_t;
public:
    explicit		Window (const Msg::Link& l);
    bool		dispatch (Msg& msg) override;
    virtual void	draw (void);
    virtual void	on_event (const Event& ev);
    virtual void	on_modified (widgetid_t, const string_view&) {}
    virtual void	on_key (key_t key);
    void		close (void);
    void		PWidget_event (const Event& ev)	{ on_event (ev); }
    void		PWidgetR_event (const Event& ev){ on_event (ev); }
    void		PWidgetR_modified (widgetid_t wid, const string_view& t)
			    { on_modified (wid, t); }
    void		ScreenR_event (const Event& ev)	{ on_event (ev); }
    void		ScreenR_expose (void)		{ draw(); }
    void		ScreenR_restate (const Info& wi)	{ _info = wi; layout(); }
    void		ScreenR_screen_info (const ScreenInfo& scrinfo);
    auto&		window_info (void) const	{ return _info; }
    auto&		screen_info (void) const	{ return _scrinfo; }
    auto&		area (void) const		{ return window_info().area(); }
protected:
    virtual void	layout (void);
    void		create_widgets (const Layout* f, const Layout* l);
    template <unsigned N>
    void		create_widgets (const Layout (&l)[N])
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
    PScreen		_scr;
    Info		_info;
    ScreenInfo		_scrinfo;
    widgetid_t		_focused;
    unique_ptr<Widget>	_widgets;
};

} // namespace ui
} // namespace cwiclo
