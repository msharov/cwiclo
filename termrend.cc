// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "termrend.h"

namespace cwiclo {
namespace ui {

//{{{ TerminalWindowRenderer -------------------------------------------

TerminalWindowRenderer::TerminalWindowRenderer (void)
:_w()
,_area{}
,_viewport{}
{
}

void TerminalWindowRenderer::destroy (void)
{
    if (auto w = exchange(_w,nullptr); w)
	delwin(w);
}

//}}}-------------------------------------------------------------------
//{{{ Sizing and layout

void TerminalWindowRenderer::resize (const Rect& r)
{
    _area = r;
    if (auto ow = exchange (_w, newwin (r.h,r.w,r.y,r.x)); ow)
	delwin (ow);
    leaveok (_w, true);
    keypad (_w, true);
    meta (_w, true);
    nodelay (_w, true);
}

//}}}-------------------------------------------------------------------
//{{{ Drawing operations

void TerminalWindowRenderer::draw_color (IColor c)
{
    if (c == IColor::DefaultBold)
	wattron (_w, A_BOLD);
    else if (c == IColor::DefaultBackground)
	wattron (_w, A_REVERSE);
    else
	wattroff (_w, A_BOLD| A_REVERSE);
}

void TerminalWindowRenderer::fill_color (IColor c)
{
    if (c == IColor::DefaultBold)
	wattron (_w, A_BLINK);
    else if (c == IColor::DefaultForeground)
	wattron (_w, A_REVERSE);
    else
	wattroff (_w, A_BLINK| A_REVERSE);
}

unsigned TerminalWindowRenderer::clip_dx (unsigned dx) const
    { return min (dx, _viewport.x + _viewport.w - getcurx(_w)); }
unsigned TerminalWindowRenderer::clip_dy (unsigned dx) const
    { return min (dx, _viewport.y + _viewport.h - getcury(_w)); }

void TerminalWindowRenderer::hline (unsigned n, wchar_t c)
{
    whline (_w, c, clip_dx(n));
}

void TerminalWindowRenderer::vline (unsigned n, wchar_t c)
{
    wvline (_w, c, clip_dy(n));
}

void TerminalWindowRenderer::box (dim_t w, dim_t h)
{
    auto tlx = getcurx(_w);
    auto tly = getcury(_w);
    hline (w, 0);
    vline (h, 0);
    addch (ACS_ULCORNER);
    move_to (tlx+w-1, tly);
    vline (h, 0);
    addch (ACS_URCORNER);
    move_to (tlx, tly+h-1);
    hline (w, 0);
    addch (ACS_LLCORNER);
    move_to (tlx+w-1, tly+h-1);
    addch (ACS_LRCORNER);
    move_to (tlx, tly);
}

void TerminalWindowRenderer::bar (dim_t w, dim_t h)
{
    for (dim_t y = 0; y < h; ++y) {
	hline (w, ' ');
	move_by (0, 1);
    }
    move_by (0, -h);
}

void TerminalWindowRenderer::erase (void)
{
    draw_color (IColor::DefaultForeground);
    fill_color (IColor::DefaultBackground);
    move_to (_viewport.xy);
    bar (_viewport.w, _viewport.h);
}

void TerminalWindowRenderer::clear (void)
{
    _viewport = _area;
    werase (_w);
}

void TerminalWindowRenderer::panel (const Size& wh, PanelType t)
{
    if (t == PanelType::Raised || t == PanelType::Button) {
	bar (wh.w, wh.h);
	addch ('[');
	move_by (wh.w-2, 0);
	addch (']');
	move_by (-wh.w, 0);
    } else if (t == PanelType::PressedButton) {
	bar (wh.w, wh.h);
	addch ('>');
	move_by (wh.w-2, 0);
	addch ('<');
	move_by (-wh.w, 0);
    } else if (t == PanelType::Sunken || t == PanelType::Editbox) {
	wattron (_w, A_UNDERLINE);
	bar (wh.w, wh.h);
	wattroff (_w, A_UNDERLINE);
    } else if (t == PanelType::Selection || t == PanelType::StatusBar) {
	wattron (_w, A_REVERSE);
	bar (wh.w, wh.h);
	wattroff (_w, A_REVERSE);
    }
}

//}}}-------------------------------------------------------------------
//{{{ Drawlist dispatch

void TerminalWindowRenderer::Draw_line (const Offset& o)
{
    if (o.dy)
	vline (o.dy, ACS_VLINE);
    else
	hline (o.dx, ACS_HLINE);
}

void TerminalWindowRenderer::Draw_text (const string& t, HAlign ha, VAlign)
{
    auto tx = getcurx(_w);
    auto ty = getcury(_w);
    for (auto l = t.begin(), tend = t.end(); l < tend; ++ty) {
	auto lend = t.find ('\n', l);
	if (!lend)
	    lend = tend;
	auto n = lend-l;

	auto loff = 0;
	if (ha == HAlign::Center)
	    loff = divide_ceil(n,2);
	else if (ha == HAlign::Right)
	    loff = n;

	wmove (_w, ty, tx-loff);
	auto attrs = winch (_w);
	attrs &= A_ATTRIBUTES| A_COLOR;
	wattron (_w, attrs);
	waddnstr (_w, l, clip_dx(n));
	wattroff (_w, attrs);

	// End position for Center and Right alignments is the left edge
	// taking advantage of text measurement needed to draw the text.
	if (ha != HAlign::Left)
	    wmove (_w, ty, tx-loff);

	l = lend+1;
    }
}


//}}}-------------------------------------------------------------------

} // namespace ui
} // namespace cwiclo
