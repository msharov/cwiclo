// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

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
    _scr.close();
    set_unused();
}

void Window::on_msger_destroyed (mrid_t mid)
{
    if (mid == _scr.dest())
	set_unused();
    Msger::on_msger_destroyed (mid);
}

void Window::create_widgets (const Widget::Layout* f, const Widget::Layout* l)
{
    if (f >= l)
	return;
    _widgets = Widget::create (this, *f);
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
    draw();
}

void Window::focus_next (void)
{
    if (_widgets)
	focus_widget (_widgets->next_focus (focused_widget_id()));
}

void Window::focus_prev (void)
{
    if (_widgets)
	focus_widget (_widgets->prev_focus (focused_widget_id()));
}

//}}}-------------------------------------------------------------------
//{{{ Layout

Rect Window::compute_size_hints (void) const
{
    Rect wh (_scrinfo.size());
    if (_widgets)
	wh = _widgets->compute_size_hints();
    // Stretch, if requested
    if (wh.x) {
	wh.w = _scrinfo.size().w;
	wh.x = 0;
    }
    if (wh.y) {
	wh.h = _scrinfo.size().h;
	wh.y = 0;
    }
    return wh;
}

void Window::layout (void)
{
    _scr.open (Info (Info::Type::Normal, compute_size_hints()));
}

void Window::on_resize (void)
{
    if (_widgets) {
	// Size hints must now be recomputed as they may depend on
	// screen info, which is usually set just before this call.
	_widgets->compute_size_hints();
	// Layout subwidgets in the computed area
	_widgets->resize (Rect (area().size()));
	if (!focused_widget_id())
	    focus_next();	// initialize focus if needed
    }
    draw();
}

void Window::ScreenR_resize (const Info& wi)
{
    _info = wi;
    on_resize();
}

//}}}-------------------------------------------------------------------
//{{{ Drawing

void Window::draw (void)
{
    if (flag (f_DrawInProgress)) {
	set_flag (f_DrawPending);
	if (!_scr.has_outgoing_draw())
	    return;
    }
    set_flag (f_DrawPending, false);
    set_flag (f_DrawInProgress);
    auto dl = _scr.begin_draw();
    on_draw (dl);
    if (_widgets)
	_widgets->draw (dl);
    _scr.end_draw (move(dl));
}

//}}}-------------------------------------------------------------------
//{{{ Event handling

void Window::on_event (const Event& ev)
{
    // KeyDown events go only to the focused widget
    if (ev.type() == Event::Type::KeyDown && (focused_widget_id() == wid_None || ev.src() != wid_None))
	on_key (ev.key());	// key events unused by widgets are processed in the window handler
    else if (ev.type() == Event::Type::Close)
	close();
    else if (ev.type() == Event::Type::VSync) {
	set_flag (f_DrawInProgress, false);
	if (flag (f_DrawPending))
	    draw();
    } else if (_widgets)
	_widgets->on_event (ev);
}

void Window::on_key (key_t k)
{
    if (k == Key::Tab || k == Key::Right || k == Key::Down)
	focus_next();
    else if (k == KMod::Shift+Key::Tab || k == Key::Left || k == Key::Up)
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
