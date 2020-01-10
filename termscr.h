// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "widget.h"
#include "app.h"
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

class TerminalScreenWindow;

class TerminalScreen : public Msger {
public:
    static auto& instance (void) { static TerminalScreen s_scr; return s_scr; }
    void	register_window (TerminalScreenWindow* w);
    void	unregister_window (const TerminalScreenWindow* w);
    void	create_window (WindowInfo& winfo, WINDOW*& pwin);
    bool	dispatch (Msg& msg) override;
    void	TimerR_timer (PTimerR::fd_t);
    auto&	screen_info (void) const { return _scrinfo; }
protected:
		TerminalScreen (void);
		~TerminalScreen (void) override;
private:
    vector<TerminalScreenWindow*> _windows;
    ScreenInfo	_scrinfo;
    PTimer	_uiinput;
};

class TerminalScreenWindow : public Msger {
public:
    enum { COLOR_DEFAULT = -1 };	// Curses transparent color
    enum { f_CaretOn = Msger::f_Last, f_Last };
public:
		TerminalScreenWindow (const Msg::Link& l);
		~TerminalScreenWindow (void) override;
    bool	dispatch (Msg& msg) override;
    void	Screen_open (const WindowInfo& wi);
    void	Screen_close (void);
    void	Screen_draw (const cmemlink& dl);
    void	Screen_get_info (void);
    auto&	screen_info (void) const	{ return TerminalScreen::instance().screen_info(); }
    auto&	window_info (void) const	{ return _winfo; }
    auto&	area (void) const		{ return window_info().area(); }
    auto&	viewport (void) const		{ return _viewport; }
    void	on_event (const Event& ev);
    void	draw_color (IColor c);
    void	fill_color (IColor c);
    void	move_to (coord_t x, coord_t y)	{ wmove (_w, viewport().y+y, viewport().x+x); }
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
    PScreenR	_reply;
    WINDOW*	_w;
    Rect	_viewport;
    WindowInfo	_winfo;
};

} // namespace ui
} // namespace cwiclo
