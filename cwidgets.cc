// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#if __has_include(<curses.h>)
#include "cwidgets.h"
#include <ctype.h>

namespace cwiclo {
namespace ui {

//{{{ Label ------------------------------------------------------------

void Label::on_draw (void) const
{
    Widget::on_draw();
    waddstr (w(), text().c_str());
}

//}}}-------------------------------------------------------------------
//{{{ Button

void Button::on_draw (void) const
{
    Widget::on_draw();
    if (focused())
	wattron (w(), A_REVERSE);
    wattron (w(), A_BOLD);
    waddstr (w(), "[ ");
    waddch (w(), text()[0]);
    wattroff (w(), A_BOLD);
    waddstr (w(), &text()[1]);
    wattron (w(), A_BOLD);
    waddstr (w(), " ]");
    wattroff (w(), A_BOLD| A_REVERSE);
}

void Button::on_set_text (void)
{
    Widget::on_set_text();
    set_size_hints (strlen("[ ") + size_hints().w + strlen (" ]"), size_hints().h);
}

//}}}-------------------------------------------------------------------
//{{{ Listbox

void Listbox::on_set_text (void)
{
    set_n (zstr::nstrs (text().c_str(), text().size()));
    clip_sel();
}

void Listbox::on_draw (void) const
{
    Widget::on_draw();
    coord_t y = 0;
    for (zstr::cii li (text().c_str(), text().size()); li; ++y) {
	auto lt = *li;
	auto tlen = *++li - lt - 1;
	if (y < _top)
	    continue;
	wmove (w(), y-_top, 0);
	if (y == selection_start() && focused()) {
	    wattron (w(), A_REVERSE);
	    whline (w(), ' ', maxx());
	}
	auto vislen = tlen;
	if (tlen > maxx())
	    vislen = maxx()-1;
	waddnstr (w(), lt, vislen);
	if (tlen > maxx())
	    waddch (w(), '>');
	if (y == selection_start())
	    wattroff (w(), A_REVERSE);
    }
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

//}}}-------------------------------------------------------------------
//{{{ Editbox

void Editbox::on_resize (void)
{
    Widget::on_resize();
    leaveok (w(), false);
    wbkgdset (w(), ' '|A_UNDERLINE);
    posclip();
}

void Editbox::posclip (void)
{
    if (_cpos < _fc) {	// cursor past left edge
	_fc = _cpos;
	if (_fc > 0)
	    --_fc;	// adjust for clip indicator
    }
    if (w()) {
	if (_fc+maxx() < _cpos+2)	// cursor past right edge +2 for > indicator
	    _fc = _cpos+2-maxx();
	while (_fc && _fc-1u+maxx() > text().size())	// if text fits in box, no scroll
	    --_fc;
    }
}

void Editbox::on_set_text (void)
{
    _cpos = text().size();
    _fc = 0;
    posclip();
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

void Editbox::on_draw (void) const
{
    Widget::on_draw();
    waddstr (w(), text().iat(_fc));
    if (_fc)
	mvwaddch (w(), 0, 0, '<');
    if (_fc+maxx() <= text().size())
	mvwaddch (w(), 0, maxx()-1, '>');
    wmove (w(), 0, _cpos-_fc);
}

//}}}-------------------------------------------------------------------

} // namespace ui
} // namespace cwiclo
#endif // __has_include(<curses.h>)
