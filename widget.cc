// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#if __has_include(<curses.h>)
#include "widget.h"

namespace cwiclo {
namespace ui {

Widget::Widget (const Msg::Link& l, const Layout& lay)
:_text()
,_w()
,_area()
,_size_hints()
,_selection()
,_flags()
,_layinfo(lay)
,_reply (l)
{
}

void Widget::resize (int l, int c, int y, int x)
{
    set_area (x, y, c, l);
    if (layinfo().type == Type::HBox || layinfo().type == Type::VBox)
	return;	// packing boxes have no window
    auto ow = exchange (_w, newwin (l,c,y,x));
    if (ow)
	delwin (ow);
    leaveok (w(), true);
    keypad (w(), true);
    meta (w(), true);
    nodelay (w(), true);
    on_resize();
}

auto Widget::measure_text (const string_view& text) -> Size
{
    Size sz = {};
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

void Widget::report_modified (void)
{
    _reply.modified (widget_id(), text());
}

void Widget::on_set_text (void)
{
    set_size_hints (measure_text (text()));
}

void Widget::on_event (const Event& ev)
{
    if (ev.type() == Event::Type::KeyDown)
	on_key (ev.key());
}

void Widget::on_key (key_t k)
{
    create_event (Event (Event::Type::KeyDown, k, widget_id()));
}

} // namespace ui
} // namespace cwiclo
#endif // __has_include(<curses.h>)
