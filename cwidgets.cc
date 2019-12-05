// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#if __has_include(<curses.h>)
#include "cwidgets.h"
#include <ctype.h>
#include <curses.h>
#undef vline
#undef hline
#undef box

namespace cwiclo {
namespace ui {

//{{{ Label ------------------------------------------------------------

DEFINE_WIDGET_WRITE_DRAWLIST (Label, Drawlist, drw)
    { drw.text (text()); }

//}}}-------------------------------------------------------------------
//{{{ Button

void Button::on_set_text (void)
{
    Widget::on_set_text();
    set_size_hints (strlen("[ ") + size_hints().w + strlen (" ]"), size_hints().h);
}

DEFINE_WIDGET_WRITE_DRAWLIST (Button, Drawlist, drw)
{
    if (focused())
	drw.fill_color (IColor::DefaultForeground);
    drw.panel (area().wh, PanelType::Button);
    drw.move_by (2, 0);
    drw.draw_color (IColor::DefaultBold);
    if (!text().empty()) {
	drw.text (text()[0]);
	drw.draw_color (IColor::DefaultForeground);
	drw.text (text().iat(1));
    }
}

//}}}-------------------------------------------------------------------
//{{{ Listbox

void Listbox::on_set_text (void)
{
    set_n (zstr::nstrs (text().c_str(), text().size()));
    clip_sel();
}

void Listbox::on_key (key_t k)
{
    if ((k == 'k' || k == KEY_UP) && selection_start())
	set_selection (selection_start()-1);
    else if ((k == 'j' || k == KEY_DOWN) && selection_start()+1 < _n)
	set_selection (selection_start()+1);
    else
	return Widget::on_key (k);
    report_selection();
}

DEFINE_WIDGET_WRITE_DRAWLIST (Listbox, Drawlist, drw)
{
    drw.panel (area().wh, PanelType::Listbox);
    coord_t y = 0;
    for (zstr::cii li (text().c_str(), text().size()); li; ++y) {
	auto lt = *li;
	auto tlen = *++li - lt - 1;
	if (y < _top)
	    continue;
	drw.move_to (0, y-_top);
	if (y == selection_start() && focused())
	    drw.panel (area().w, 1, PanelType::Selection);
	auto vislen = tlen;
	if (tlen > area().w)
	    vislen = area().w-1;
	drw.text (lt, vislen);
	if (tlen > area().w)
	    drw.text ('>');
    }
}

//}}}-------------------------------------------------------------------
//{{{ Editbox

Editbox::Editbox (const Msg::Link& l, const Layout& lay)
: Widget(l,lay)
,_cpos()
,_fc()
{
    set_flag (f_HasCaret);
    set_flag (f_CanFocus);
    set_size_hints (0, 1);
}

void Editbox::on_resize (void)
{
    Widget::on_resize();
    posclip();
}

void Editbox::posclip (void)
{
    if (_cpos < _fc) {	// cursor past left edge
	_fc = _cpos;
	if (_fc > 0)
	    --_fc;	// adjust for clip indicator
    }
    if (area().w) {
	if (_fc+area().w < _cpos+2)	// cursor past right edge +2 for > indicator
	    _fc = _cpos+2-area().w;
	while (_fc && _fc-1u+area().w > text().size())	// if text fits in box, no scroll
	    --_fc;
    }
}

void Editbox::on_set_text (void)
{
    _cpos = text().size();
    _fc = 0;
    posclip();
    // An edit box must be one line only
    set_size_hints (size_hints().w, 1);
}

void Editbox::on_key (key_t k)
{
    if (k == KEY_LEFT && _cpos)
	--_cpos;
    else if (k == KEY_RIGHT && _cpos < coord_t (text().size()))
	++_cpos;
    else if (k == KEY_HOME)
	_cpos = 0;
    else if (k == KEY_END)
	_cpos = text().size();
    else {
	if ((k == KEY_BKSPACE || k == KEY_BACKSPACE) && _cpos)
	    textw().erase (text().iat(--_cpos));
	else if (k == KEY_DC && _cpos < coord_t (text().size()))
	    textw().erase (text().iat(_cpos));
	else if (isprint(k))
	    textw().insert (text().iat(_cpos++), char(k));
	else
	    return Widget::on_key (k);
	report_modified();
    }
    posclip();
}

DEFINE_WIDGET_WRITE_DRAWLIST (Editbox, Drawlist, drw)
{
    drw.panel (area().wh, PanelType::Editbox);
    if (!text().empty())
	drw.text (text().iat(_fc));
    if (_fc) {
	drw.move_to (0, 0);
	drw.text ('<');
    }
    if (_fc+area().w <= text().size()) {
	drw.move_to (area().w-1, 0);
	drw.text ('>');
    }
    drw.move_to (_cpos-_fc, 0);
}

//}}}-------------------------------------------------------------------
//{{{ HSplitter

DEFINE_WIDGET_WRITE_DRAWLIST (HSplitter, Drawlist, drw)
    { drw.hline (area().w); }

//}}}-------------------------------------------------------------------
//{{{ VSplitter

DEFINE_WIDGET_WRITE_DRAWLIST (VSplitter, Drawlist, drw)
    { drw.vline (area().h); }

//}}}-------------------------------------------------------------------
//{{{ GroupFrame

DEFINE_WIDGET_WRITE_DRAWLIST (GroupFrame, Drawlist, drw)
{
    drw.box (area().wh);
    auto tsz = min (text().size(), area().w-2);
    if (tsz > 0) {
	drw.move_to ((area().w-tsz)/2u-1, 0);
	drw.bar (tsz+2, 1);
	drw.move_by (1, 0);
	drw.text (text().c_str(), tsz);
    }
}

//}}}-------------------------------------------------------------------
//{{{ StatusLine

DEFINE_WIDGET_WRITE_DRAWLIST (StatusLine, Drawlist, drw)
{
    drw.panel (area(), PanelType::StatusBar);
    drw.move_by (1, 0);
    drw.text (text());
    if (is_modified()) {
	drw.move_to (area().w-strlen(" *"), 0);
	drw.text (" *");
    }
}

//}}}-------------------------------------------------------------------
//{{{ default_widget_factory

Widget* default_widget_factory (mrid_t owner, const Widget::Layout& lay)
{
    const Msg::Link ol { owner, owner };
    switch (lay.type()) {
	case Widget::Type::Label:	return new Label (ol,lay);
	case Widget::Type::Button:	return new Button (ol,lay);
	case Widget::Type::Listbox:	return new Listbox (ol,lay);
	case Widget::Type::Editbox:	return new Editbox (ol,lay);
	case Widget::Type::HSplitter:	return new HSplitter (ol,lay);
	case Widget::Type::VSplitter:	return new VSplitter (ol,lay);
	case Widget::Type::GroupFrame:	return new GroupFrame (ol,lay);
	case Widget::Type::StatusLine:	return new StatusLine (ol,lay);
	default:			return new Widget (ol,lay);
    }
}

// default_widget_factory set to default
Widget::widget_factory_t Widget::s_factory = default_widget_factory;

//}}}-------------------------------------------------------------------

} // namespace ui
} // namespace cwiclo
#endif // __has_include(<curses.h>)
