// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "widget.h"
#include "window.h"

namespace cwiclo {
namespace ui {

//{{{ Widget creation --------------------------------------------------

Widget::Widget (Window* w, const Layout& lay)
:_text()
,_widgets()
,_win (w)
,_area()
,_size_hints()
,_selection()
,_flags()
,_nexp{}
,_layinfo(lay)
{
}

const Widget* Widget::widget_by_id (widgetid_t id) const
{
    if (id == wid_None)
	return nullptr;
    if (widget_id() == id)
	return this;
    for (auto& w : _widgets)
	if (auto sw = w->widget_by_id(id); sw)
	    return sw;
    return nullptr;
}

const Widget::Layout* Widget::add_widgets (const Layout* f, const Layout* l)
{
    while (f < l && f->level() > layinfo().level()) {
	auto& w = _widgets.emplace_back (create (_win, *f));
	auto nf = next(f);
	if (nf < l && nf->level() > f->level())
	    nf = w->add_widgets (nf, l);
	f = nf;
    }
    return f;
}

Widget* Widget::replace_widget (unique_ptr<Widget>&& nw)
{
    assert (nw->widget_id() && "can only replace a widget with an assigned id");
    if (!nw->widget_id())
	return nullptr;
    for (auto& w : _widgets) {
	if (w->widget_id() == nw->widget_id())
	    return (w = move (nw)).get();
	else if (auto nwp = w->replace_widget (move (nw)); nwp)
	    return nwp;
    }
    return nullptr;
}

//}}}-------------------------------------------------------------------
//{{{ Sizing and layout

auto Widget::measure_text (const string_view& text) -> Size
{
    Size sz;
    for (auto l = text.begin(), textend = text.end(); l < textend;) {
	auto lend = text.find ('\n', l);
	if (!lend)
	    lend = textend;
	sz.w = max (sz.w, lend-l);
	++sz.h;
	l = lend+1;
    }
    return sz;
}

Rect Widget::compute_size_hints (void)
{
    // The returned size hints contain w/h size and x/y zero sub size counts
    if (_widgets.empty())
	return Rect (Point(_nexp.x,_nexp.y), size_hints());
    // Iterate over all widgets on the same level
    Rect rsh;
    for (auto& w : _widgets) {
	// Get the size hints and update the aggregate
	auto sh = w->compute_size_hints();

	// Count expandables
	if (sh.x || !sh.w)
	    ++rsh.x;
	if (sh.y || !sh.h)
	    ++rsh.y;

	// Packers add up widget sizes in one direction, expand to fit them in the other
	if (layinfo().type() == Type::HBox) {
	    rsh.w += sh.w;
	    rsh.h = max (rsh.h, sh.h);
	} else {
	    rsh.w = max (rsh.w, sh.w);
	    rsh.h += sh.h;
	}
    }
    // Frames add border thickness after all the subwidgets have been collected
    if (layinfo().type() == Type::GroupFrame) {
	rsh.w += 2;
	rsh.h += 2;
    }
    // Save expandables count and size hints for resize
    _nexp.x = rsh.x;
    _nexp.y = rsh.y;
    if (!flag (f_ForcedSizeHints))
	_size_hints = rsh.size();
    return rsh;
}

void Widget::resize (const Rect& inarea)
{
    // With expandable size extracted, can place the widget where indicated.
    set_area (inarea);
    on_resize();

    // Now for the subwidgets, if there are any.
    auto subpos = inarea.pos();
    auto subsz = inarea.size();
    // Group frame starts by offsetting the frame
    if (layinfo().type() == Type::GroupFrame) {
	++subpos.x;
	++subpos.y;
	subsz.w -= min (subsz.w, 2);	// may be truncated
	subsz.h -= min (subsz.h, 2);
    }
    // Compute extra size for expandables
    Size extra;
    if (size_hints().w < inarea.w)
	extra.w = inarea.w - size_hints().w;
    // If no expandables, pad to align
    if (!_nexp.x) {
	// padding may be negative if inarea is smaller than fixed size hints
	coord_t padding = inarea.w - size_hints().w;
	if (layinfo().halign() == HAlign::Right)
	    subpos.x += padding;
	else if (layinfo().halign() == HAlign::Center)
	    subpos.x += padding/2;
    }
    // Same for y
    if (size_hints().h < inarea.h)
	extra.h = inarea.h - size_hints().h;
    if (!_nexp.y) {
	coord_t padding = inarea.h - size_hints().h;
	if (layinfo().halign() == HAlign::Right)
	    subpos.y += padding;
	else if (layinfo().halign() == HAlign::Center)
	    subpos.y += padding/2;
    }

    // Starting with the one after f. If there are no subwidgets, return.
    unsigned nexpx = _nexp.x, nexpy = _nexp.y;
    for (auto& w : _widgets) {
	// Widget position already computed
	auto warea = Rect (subpos, w->size_hints());
	// See if this subwidget gets extra space
	bool xexp = w->expandables().x || !warea.w;
	bool yexp = w->expandables().y || !warea.h;

	// Packer-type dependent area computation and subpos adjustment
	if (layinfo().type() == Type::HBox) {
	    auto sw = min (subsz.w, warea.w);
	    // Add extra space, if available
	    if (xexp && nexpx) {
		auto ew = extra.w/nexpx--;	// divided equally between expandable subwidgets
		extra.w -= ew;
		sw += ew;
	    }
	    subpos.x += sw;
	    subsz.w -= sw;
	    warea.w = sw;
	    warea.h = subsz.h;
	} else {
	    warea.w = subsz.w;
	    auto sh = min (subsz.h, warea.h);
	    if (yexp && nexpy) {
		auto eh = extra.h/nexpy--;
		extra.h -= eh;
		sh += eh;
	    }
	    subpos.y += sh;
	    subsz.h -= sh;
	    warea.h = sh;
	}

	// Recurse to layout w.
	w->resize (warea);
    }
}

//}}}-------------------------------------------------------------------
//{{{ Content

PWidgetR Widget::widget_reply (void) const
    { return PWidgetR (parent_window()->msger_id(), parent_window()->msger_id()); }
void Widget::create_event (const Event& ev) const
    { widget_reply().event (ev); }
void Widget::report_selection (void) const
    { create_event (Event (Event::Type::Selection, _selection, 0, widget_id())); }

void Widget::report_modified (void) const
{
    widget_reply().modified (widget_id(), text());
}

void Widget::draw (drawlist_t& dl) const
{
    on_draw (dl);
    for (auto& w : _widgets)
	w->draw (dl);
}

//}}}-------------------------------------------------------------------
//{{{ Focus

void Widget::get_focus_neighbors_for (widgetid_t wid, FocusNeighbors& n) const
{
    if (widget_id() != wid_None && flag (f_CanFocus)) {
	n.first = min (n.first, widget_id());
	n.last = max (n.last, widget_id());
	// next is after wid, but closer than n.next
	if (widget_id() > wid && widget_id() < n.next)
	    n.next = widget_id();
	if (widget_id() < wid && widget_id() > n.prev)
	    n.prev = widget_id();
    }
    for (auto& w : _widgets)
	w->get_focus_neighbors_for (wid, n);
}

auto Widget::get_focus_neighbors_for (widgetid_t wid) const -> FocusNeighbors
{
    FocusNeighbors n = { wid_Last, wid_None, wid_Last, wid_None };
    get_focus_neighbors_for (wid, n);
    // No widgets - n.first should be wid_None
    if (n.first == wid_Last)
	n.first = wid_None;
    // Wrap around next/prev when at ends
    if (n.prev == wid_None)
	n.prev = n.last;
    if (n.next == wid_Last)
	n.next = n.first;
    return n;
}

void Widget::focus (widgetid_t id)
{
    bool f = (widget_id() == id && flag (f_CanFocus));
    for (auto& w : _widgets) {
	w->focus (id);
	if (w->flag (f_Focused))
	    f = true;
    }
    set_flag (f_Focused, f);
}

//}}}-------------------------------------------------------------------
//{{{ Event handling

void Widget::on_event (const Event& ev)
{
    // KeyDown events go only to the focused widget
    if (ev.type() == Event::Type::KeyDown) {
	// on_key handlers are called in leaves and in focusable containers
	if (flag (f_CanFocus))	// only call if the widget is able to handle it
	    on_key (ev.key());
	// Key events that the focused widget does not use are forwarded
	// back here with the source widget id set to that widget.
	auto focusw = linear_search_if (_widgets, [](auto& w) { return w->focused(); });
	if (focusw)
	    (*focusw)->on_event (ev);
    } else
	for (auto& w : _widgets)
	    w->on_event (ev);
}

void Widget::on_key (key_t k)
{
    // At the end of the focus path, return unused key events to parent window.
    // Retuning directly to on_event to avoid sequencing problems with returned
    // and directly handled key events.
    if (widget_id() && _widgets.empty() && flag (f_CanFocus))
	_win->on_event (Event (Event::Type::KeyDown, k, widget_id()));
}

//}}}-------------------------------------------------------------------

} // namespace ui
} // namespace cwiclo
