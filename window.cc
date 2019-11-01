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

//----------------------------------------------------------------------

mrid_t CursesWindow::s_focused = 0;
uint8_t CursesWindow::s_nwins = 0;

//----------------------------------------------------------------------

CursesWindow::CursesWindow (const Msg::Link& l)
: Msger(l)
,_winput()
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

CursesWindow::~CursesWindow (void)
{
    if (!--s_nwins) {
	flushinp();
	endwin();
    }
}

auto CursesWindow::widget_by_id (widgetid_t id) const -> const Widget*
{
    if (id)
	for (auto& w : _widgets)
	    if (w->widget_id() == id)
		return w.get();
    return nullptr;
}

auto CursesWindow::create_widget (const Widget::Layout& lay) -> Widget*
{
    Widget* w;
    Msg::Link l { msger_id(), msger_id() };
    switch (lay.type) {
	default:			w = new Widget (l,lay);		break;
	case Widget::Type::Label:	w = new Label (l,lay);		break;
	case Widget::Type::Button:	w = new Button (l,lay);		break;
	case Widget::Type::Listbox:	w = new Listbox (l,lay);	break;
	case Widget::Type::Editbox:	w = new Editbox (l,lay);	break;
	case Widget::Type::HSplitter:	w = new HSplitter (l,lay);	break;
	case Widget::Type::VSplitter:	w = new VSplitter (l,lay);	break;
	case Widget::Type::GroupFrame:	w = new GroupFrame (l,lay);	break;
	case Widget::Type::StatusLine:	w = new StatusLine (l,lay);	break;
    }
    if (w)
	_widgets.emplace_back (w);
    return w;
}

void CursesWindow::create_widgets (const Widget::Layout* l, unsigned sz)
{
    for (; sz; ++l, --sz)
	create_widget (*l);
}

void CursesWindow::focus_widget (widgetid_t id)
{
    auto newf = widget_by_id (id);
    if (!newf || !newf->flag (Widget::f_CanFocus) || !newf->w())
	return;
    auto oldf = focused_widget();
    if (oldf)
	oldf->set_flag (Widget::f_Focused, false);
    newf->set_flag (Widget::f_Focused);
    _focused = newf->widget_id();
    _winput = newf->w();
    bool newcaret = newf->flag (Widget::f_HasCaret);
    if (!oldf || oldf->flag (Widget::f_HasCaret) != newcaret) {
	set_flag (f_CaretOn, newcaret);
	curs_set (newcaret);
    }
    s_focused = msger_id();
    TimerR_timer (STDIN_FILENO);
}

void CursesWindow::focus_next (void)
{
    widgetid_t oid = focused_widget_id(), nid = wid_Last, fid = wid_Last;
    for (auto& w : _widgets) {
	if (!w->widget_id() || !w->flag (Widget::f_CanFocus) || !w->w())
	    continue;			// Focusable widgets must set flag, have id, and be laid out
	fid = min (fid, w->widget_id());	// widget with lowest id for wraparound
	if (w->widget_id() > oid && w->widget_id() < nid)
	    nid = w->widget_id();		// pick the closest next id
    }
    if (nid == wid_Last)
	nid = fid;	// If nothing found, wrap around to first
    focus_widget (nid);
}

void CursesWindow::focus_prev (void)
{
    widgetid_t oid = focused_widget_id(), nid = wid_None, lid = wid_None;
    for (auto& w : _widgets) {
	if (!w->widget_id() || !w->flag (Widget::f_CanFocus) || !w->w())
	    continue;			// Focusable widgets must set flag, have id, and be laid out
	lid = max (lid, w->widget_id());	// widget with highest id for wraparound
	if (w->widget_id() < oid && w->widget_id() > nid)
	    nid = w->widget_id();		// pick the closest previous id
    }
    if (!nid)
	nid = lid;	// If nothing found, wrap around to last
    focus_widget (nid);
}

void CursesWindow::on_msger_destroyed (mrid_t mid)
{
    Msger::on_msger_destroyed (mid);
    // Check if a child modal dialog closed
    if (mid == s_focused && _winput) {
	curs_set (flag (f_CaretOn));
	s_focused = msger_id();
	TimerR_timer (STDIN_FILENO);
    }
}

auto CursesWindow::compute_size_hints (const Widget::Layout& plinfo, widgetvec_t::iterator& f, widgetvec_t::iterator l) -> Rect
{
    // The returned size hints contain w/h size and x/y zero sub size counts
    Rect rsh = {};

    // Iterate over all widgets on the same level
    while (f < l && (*f)->layinfo().level > plinfo.level) {
	// Get the size hints and update the aggregate
	Rect sh = {};
	sh.wh = (*f)->size_hints();
	// If the widget is a container, recurse to update its area
	auto nf = next(f);
	if (nf < l && (*nf)->layinfo().level > (*f)->layinfo().level)
	    sh = compute_size_hints ((*f)->layinfo(), nf, l);
	(*f)->set_area (sh);
	f = nf;	// set f to the next widget with the same level

	// Count expandables
	if (sh.x || !sh.w)
	    ++rsh.x;
	if (sh.y || !sh.h)
	    ++rsh.y;

	// Packers add up widget sizes in one direction, expand to fit them in the other
	if (plinfo.type == Widget::Type::HBox) {
	    rsh.w += sh.w;
	    rsh.h = max (rsh.h, sh.h);
	} else {
	    rsh.w = max (rsh.w, sh.w);
	    rsh.h += sh.h;
	}
    }
    // Frames add border thickness after all the subwidgets have been collected
    if (plinfo.type == Widget::Type::GroupFrame) {
	rsh.w += 2;
	rsh.h += 2;
    }
    return rsh;
}

auto CursesWindow::layout_widget (const Rect& area, widgetvec_t::iterator f, widgetvec_t::iterator l) -> widgetvec_t::iterator
{
    // area contains the final laid out size of this widget.
    // Number of expandables is in current area.xy, set in collect_size_hints.
    //
    Size fixed = (*f)->area().wh;
    unsigned nexpsx = (*f)->area().x, nexpsy = (*f)->area().y;
    // With expandable size extracted, can place the widget where indicated.
    (*f)->resize (area);

    // Current f is the parent of these, determining the pack.
    auto& plinfo = (*f)->layinfo();

    // Now for the subwidgets, if there are any.
    auto subpos = area.xy;
    auto subsz = area.wh;
    // Group frame starts by offsetting the frame
    if (plinfo.type == Widget::Type::GroupFrame) {
	++subpos.x;
	++subpos.y;
	subsz.w -= min (subsz.w, 2);	// may be truncated
	subsz.h -= min (subsz.h, 2);
    }
    // Compute the difference between that and the fixed hints to determine
    // extra size for expandables. If shrinking, apply alignment offset.
    Size extra = {};
    if (fixed.w < area.w)
	extra.w = area.w - fixed.w;
    else {
	dim_t padding = fixed.w - area.w;
	if (plinfo.halign == Widget::HAlign::Right)
	    subpos.x += padding;
	else if (plinfo.halign == Widget::HAlign::Center)
	    subpos.x += padding/2;
    }
    if (fixed.h < area.h)
	extra.h = area.h - fixed.h;
    else {
	unsigned padding = fixed.h - area.h;
	if (plinfo.halign == Widget::HAlign::Right)
	    subpos.y += padding;
	else if (plinfo.halign == Widget::HAlign::Center)
	    subpos.y += padding/2;
    }

    // Starting with the one after f. If there are no subwidgets, return.
    f = next(f);
    while (f < l && (*f)->layinfo().level > plinfo.level) {
	auto farea = (*f)->area();
	// See if this subwidget gets extra space
	bool xexp = farea.x || !farea.w;
	bool yexp = farea.y || !farea.h;
	// Widget position already computed
	farea.xy = subpos;

	// Packer-type dependent area computation and subpos adjustment
	if (plinfo.type == Widget::Type::HBox) {
	    auto sw = min (subsz.w, farea.w);
	    // Add extra space, if available
	    if (xexp && nexpsx) {
		auto ew = extra.w/nexpsx--;	// divided equally between expandable subwidgets
		extra.w -= ew;
		sw += ew;
	    }
	    subpos.x += sw;
	    subsz.w -= sw;
	    farea.w = sw;
	    farea.h = subsz.h;
	} else {
	    farea.w = subsz.w;
	    auto sh = min (subsz.h, farea.h);
	    if (yexp && nexpsy) {
		auto eh = extra.h/nexpsy--;
		extra.h -= eh;
		sh += eh;
	    }
	    subpos.y += sh;
	    subsz.h -= sh;
	    farea.h = sh;
	}

	// Recurse to layout f. This will also advance f.
	f = layout_widget (farea, f, l);
    }
    return f;
}

void CursesWindow::layout (void)
{
    static const Widget::Layout c_root_layout = { .level = 0, .type = Widget::Type::VBox, .halign = Widget::HAlign::Center, .valign = Widget::VAlign::Center };
    auto wi = _widgets.begin();
    auto wh = compute_size_hints (c_root_layout, wi, _widgets.end());

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
    layout_widget (wh, _widgets.begin(), _widgets.end());

    // Reset focus, if needed
    if (!focused_widget_id())
	focus_next();	// initialize focus if needed
    draw();
}

void CursesWindow::draw (void) const
{
    for (auto& w : _widgets)
	if (w->w())
	    w->draw();
    if (_winput)
	wnoutrefresh (_winput);	// to show cursor
    doupdate();
}

void CursesWindow::on_event (const Event& ev)
{
    // KeyDown events go only to the focused widget
    if (ev.type() == Event::Type::KeyDown) {
	// Key events that the focused widget does not use are forwarded
	// back here with the source widget id set to that widget.
	auto focusw = focused_widget();
	if (focusw && ev.src() != focusw->widget_id())
	    focusw->on_event (ev);
	else
	    on_key (ev.key());	// key events unused by widgets are processed in the window handler
    } else
	for (auto& w : _widgets)
	    if (ev.src() != w->widget_id())
		w->on_event (ev);
}

void CursesWindow::on_key (key_t k)
{
    if (k == KEY_RESIZE)
	layout();
    else if (k == '\t' || k == KEY_RIGHT || k == KEY_DOWN)
	focus_next();
    else if (k == KEY_LEFT || k == KEY_UP)
	focus_prev();
}

void CursesWindow::close (void)
{
    set_unused();
    _uiinput.stop();
}

void CursesWindow::TimerR_timer (PTimerR::fd_t)
{
    if (msger_id() != s_focused || !_winput)
	return;	// a modal dialog is active
    for (int k; 0 < (k = wgetch (_winput));)
	PWidget_event (Event (Event::Type::KeyDown, key_t(k)));
    if (!flag (f_Unused) && !isendwin()) {
	draw();
	_uiinput.wait_read (STDIN_FILENO);
    }
}

bool CursesWindow::dispatch (Msg& msg)
{
    return PTimerR::dispatch (this, msg)
	|| PWidgetR::dispatch (this, msg)
	|| Msger::dispatch (msg);
}

} // namespace ui
} // namespace cwiclo
#endif // __has_include(<curses.h>)
