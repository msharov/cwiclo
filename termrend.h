// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "widget.h"
#include <curses.h>
#undef addch
#undef addstr
#undef addnstr
#undef attron
#undef attroff
#undef hline
#undef vline
#undef box
#undef erase
#undef getch
#undef clear

namespace cwiclo {
namespace ui {

class TerminalWindowRenderer {
    enum { COLOR_DEFAULT = -1 };	// Curses transparent color
public:
		TerminalWindowRenderer (void);
		~TerminalWindowRenderer (void)	{ destroy(); }
    void	destroy (void);
    void	resize (const Rect& r);
    void	draw_color (IColor c);
    void	fill_color (IColor c);
    void	move_to (coord_t x, coord_t y)	{ wmove (_w, _viewport.y+y, _viewport.x+x); }
    void	move_to (const Point& pt)	{ move_to (pt.x, pt.y); }
    void	move_by (coord_t dx, coord_t dy){ wmove (_w, dy+getcury(_w), dx+getcurx(_w)); }
    void	move_by (const Offset& o)	{ move_by (o.dx, o.dy); }
    void	addch (wchar_t c)		{ waddch (_w, c); }
    void	hline (unsigned n, wchar_t c);
    void	vline (unsigned n, wchar_t c);
    void	box (dim_t w, dim_t h);
    void	bar (dim_t w, dim_t h);
    void	erase (void);
    void	clear (void);
    void	panel (const Size& wh, PanelType t);
    void	panel (dim_t w, dim_t h, PanelType t)	{ panel (Size{w,h}, t); }
    void	panel (const Rect& r, PanelType t)	{ move_to (r.xy); panel (r.wh, t); }
    void	noutrefresh (void)			{ wnoutrefresh (_w); }
    void	refresh (void)				{ wrefresh (_w); }
    auto	getch (void)				{ return wgetch (_w); }
		operator bool (void) const		{ return _w; }
    void	Draw_clear (void)			{ erase(); }
    void	Draw_move_to (const Point& p)		{ move_to (p); }
    void	Draw_move_by (const Offset& o)		{ move_by (o); }
    void	Draw_viewport (const Rect& vp)		{ _viewport = vp; move_to (0, 0); }
    void	Draw_line (const Offset& o);
    void	Draw_draw_color (icolor_t c)		{ draw_color (IColor(c)); }
    void	Draw_fill_color (icolor_t c)		{ fill_color (IColor(c)); }
    void	Draw_text (const string& t, HAlign ha, VAlign va);
    void	Draw_bar (const Size& wh)		{ bar (wh.w, wh.h); }
    void	Draw_box (const Size& wh)		{ box (wh.w, wh.h); }
    void	Draw_panel (const Size& wh, PanelType t){ panel (wh, t); }
private:
    unsigned	clip_dx (unsigned dx) const;
    unsigned	clip_dy (unsigned dx) const;
private:
    WINDOW*	_w;
    Rect	_area;
    Rect	_viewport;
};

} // namespace ui
} // namespace cwiclo
