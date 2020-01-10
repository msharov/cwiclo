// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#if __has_include(<curses.h>)
#include "window.h"
#include "cwidgets.h"

namespace cwiclo {
namespace ui {

//{{{ Widget creation --------------------------------------------------

Window::Window (const Msg::Link& l)
: Msger (l)
,_scr (l.dest)
,_focused (wid_None)
,_widgets()
{
    _scr.get_info();
}

void Window::close (void)
{
    set_unused();
}

void Window::create_widgets (const Widget::Layout* f, const Widget::Layout* l)
{
    if (f >= l)
	return;
    _widgets = Widget::create (msger_id(), *f);
    f = _widgets->add_widgets (next(f), l);
    assert (f == l && "Your layout array must have a single root widget containing all the others");
}

//}}}-------------------------------------------------------------------
//{{{ Focus

void Window::focus_widget (widgetid_t id)
{
    auto newf = widget_by_id (id);
    if (!newf || !newf->flag (Widget::f_CanFocus))
	return;
    _focused = id;
    _widgets->focus (id);
}

void Window::focus_next (void)
{
    focus_widget (_widgets->next_focus (focused_widget_id()));
}

void Window::focus_prev (void)
{
    focus_widget (_widgets->prev_focus (focused_widget_id()));
}

//}}}-------------------------------------------------------------------
//{{{ Layout

void Window::ScreenR_screen_info (const ScreenInfo& scrinfo)
{
    _scrinfo = scrinfo;
    auto wh = _widgets->compute_size_hints();
    // Stretch, if requested
    if (wh.x) {
	wh.w = _scrinfo.size().w;
	wh.x = 0;
    }
    if (wh.y) {
	wh.h = _scrinfo.size().h;
	wh.y = 0;
    }
    _scr.open (Info (Info::Type::Normal, wh));
}

void Window::layout (void)
{
    // Layout subwidgets in the computed area
    _widgets->place (Rect {{0,0,area().w,area().h}});
    // Reset focus, if needed
    if (!focused_widget_id())
	focus_next();	// initialize focus if needed
    draw();
}

//}}}-------------------------------------------------------------------
//{{{ Drawing

void Window::draw (void)
{
    drawlist_t dl;
    _widgets->draw (dl);
    _scr.draw (dl);
}

//}}}-------------------------------------------------------------------
//{{{ Event handling

void Window::on_event (const Event& ev)
{
    // KeyDown events go only to the focused widget
    if (ev.type() == Event::Type::KeyDown && (focused_widget_id() == wid_None || ev.src() != wid_None))
	on_key (ev.key());	// key events unused by widgets are processed in the window handler
    else
	_widgets->on_event (ev);
}

void Window::on_key (key_t k)
{
    if (k == KEY_RESIZE)
	layout();
    else if (k == '\t' || k == KEY_RIGHT || k == KEY_DOWN)
	focus_next();
    else if (k == KEY_LEFT || k == KEY_UP)
	focus_prev();
}

bool Window::dispatch (Msg& msg)
{
    return PScreenR::dispatch (this, msg)
	|| PWidgetR::dispatch (this, msg)
	|| Msger::dispatch (msg);
}

//}}}-------------------------------------------------------------------

} // namespace ui
} // namespace cwiclo
#endif // __has_include(<curses.h>)
