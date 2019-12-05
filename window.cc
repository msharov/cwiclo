// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#if __has_include(<curses.h>)
#include "window.h"
#include "cwidgets.h"
#include <signal.h>

namespace cwiclo {
namespace ui {

//{{{ Statics ----------------------------------------------------------

mrid_t Window::s_focused = 0;
uint8_t Window::s_nwins = 0;

//}}}-------------------------------------------------------------------
//{{{ Widget creation

Window::Window (const Msg::Link& l)
: Msger(l)
,_w()
,_uiinput (l.dest)
,_focused (wid_None)
,_widgets()
{
    if (s_nwins++)
	return;
    if (!initscr()) {
	error ("unable to initialize terminal output");
	return;
    }
    start_color();
    use_default_colors();
    noecho();
    cbreak();
    curs_set (false);
    ESCDELAY = 10;		// reduce esc key compose wait to 10ms
    signal (SIGTSTP, SIG_IGN);	// disable Ctrl-Z
}

Window::~Window (void)
{
    if (!--s_nwins) {
	flushinp();
	endwin();
    }
}

void Window::close (void)
{
    set_unused();
    _uiinput.stop();
}

void Window::on_msger_destroyed (mrid_t mid)
{
    Msger::on_msger_destroyed (mid);
    // Check if a child modal dialog closed
    if (mid == s_focused && w()) {
	curs_set (flag (f_CaretOn));
	s_focused = msger_id();
	TimerR_timer (STDIN_FILENO);
    }
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
    bool newcaret = newf->flag (Widget::f_HasCaret);
    auto oldf = focused_widget();
    if (!oldf || oldf->flag (Widget::f_HasCaret) != newcaret) {
	set_flag (f_CaretOn, newcaret);
	curs_set (newcaret);
    }
    s_focused = msger_id();
    TimerR_timer (STDIN_FILENO);
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

void Window::layout (void)
{
    auto wh = _widgets->compute_size_hints();

    // Compute the main window area
    if (wh.x)
	wh.w = COLS;
    else
	wh.w = min (wh.w, COLS);
    wh.x = 0;
    if (wh.w < COLS)
	wh.x += (COLS-wh.w)/2u;
    if (wh.y)
	wh.h = LINES;
    else
	wh.h = min (wh.h, LINES);
    wh.y = 0;
    if (wh.h < LINES)
	wh.y += (LINES-wh.h)/2u;

    // Layout subwidgets in the computed area
    _widgets->place (Rect {0,0,wh.w,wh.h});
    w().resize (wh);

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
    w().clear();
    Drawlist::dispatch (&w(), dl);
    w().refresh();
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

void Window::TimerR_timer (PTimerR::fd_t)
{
    if (msger_id() != s_focused || !w())
	return;	// a modal dialog is active
    for (int k; 0 < (k = w().getch());)
	PWidget_event (Event (Event::Type::KeyDown, key_t(k)));
    if (!flag (f_Unused) && !isendwin()) {
	draw();
	_uiinput.wait_read (STDIN_FILENO);
    }
}

bool Window::dispatch (Msg& msg)
{
    return PTimerR::dispatch (this, msg)
	|| PWidgetR::dispatch (this, msg)
	|| Msger::dispatch (msg);
}

//}}}-------------------------------------------------------------------

} // namespace ui
} // namespace cwiclo
#endif // __has_include(<curses.h>)
