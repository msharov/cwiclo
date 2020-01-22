// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#if __has_include(<curses.h>)
#include "termscr.h"
#include "draw.h"
#include <signal.h>
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

//{{{ Statics ----------------------------------------------------------

DEFINE_INTERFACE (Screen)
DEFINE_INTERFACE (ScreenR)

//}}}-------------------------------------------------------------------
//{{{ TerminalScreen

TerminalScreen::TerminalScreen (void)
: Msger()
,_windows()
,_scrinfo()
,_uiinput (msger_id())
{
    if (!initscr()) {
	error ("unable to initialize terminal output");
	return;
    }
    _scrinfo.set_size (COLS, LINES);
    start_color();
    use_default_colors();
    noecho();
    cbreak();
    curs_set (false);
    ESCDELAY = 10;		// reduce esc key compose wait to 10ms
    signal (SIGTSTP, SIG_IGN);	// disable Ctrl-Z
}

TerminalScreen::~TerminalScreen (void)
{
    _windows.clear();
    unregister_window (nullptr);
}

void TerminalScreen::register_window (TerminalScreenWindow* w)
{
    assert (w);
    assert (!linear_search (_windows, w));
    _windows.push_back (w);
}

void TerminalScreen::unregister_window (const TerminalScreenWindow* w)
{
    remove (_windows, w);
    if (_windows.empty()) {
	_uiinput.stop();
	flushinp();
	endwin();
    } else {
	// This implementation has only one window stack,
	// with _windows.back() always the focused window.

	// Redraw all windows to erase the one that was destroyed
	werase (stdscr);
	wnoutrefresh (stdscr);
	for (auto& bw : _windows)
	    bw->draw();

	// Restart listening for keys
	TimerR_timer (STDIN_FILENO);
    }
}

void TerminalScreen::create_window (WindowInfo& winfo, WINDOW*& pwin)
{
    auto warea = winfo.area();
    // Maximize window if empty
    if (!warea.w)
	warea.w = screen_info().size().w;
    if (!warea.h)
	warea.h = screen_info().size().h;
    // Center windows that are not explicitly positioned
    if (!warea.x && !warea.y) {
	warea.w = min (warea.w, screen_info().size().w);
	warea.h = min (warea.h, screen_info().size().h);
	if (warea.w < screen_info().size().w)
	    warea.x = (screen_info().size().w - warea.w)/2u;
	if (warea.h < screen_info().size().h)
	    warea.y = (screen_info().size().h - warea.h)/2u;
    }
    winfo.set_area (warea);

    // Create or recreate curses window
    if (auto ow = exchange (pwin, newwin (warea.h,warea.w,warea.y,warea.x)); ow)
	delwin (ow);
    leaveok (pwin, true);
    keypad (pwin, true);
    meta (pwin, true);
    nodelay (pwin, true);

    // Start listening for key events
    TimerR_timer (STDIN_FILENO);
}

bool TerminalScreen::dispatch (Msg& msg)
{
    return PTimerR::dispatch (this, msg)
	|| Msger::dispatch (msg);
}

Event::key_t TerminalScreen::event_key_from_curses (int k) // static
{
    // Keys that curses should have defined, but does not
    enum {
	KEY_ESCAPE = '\e',
	KEY_BKSPACE = '~'+1
    };
    // Curses key to Event Key
    static const struct { int ck; Event::key_t ek; } c_keymap[] = {
	{ '\t',		Key::Tab	},
	{ '\n',		Key::Enter	},
	{ KEY_ESCAPE,	Key::Escape	},
	{ KEY_LEFT,	Key::Left	},
	{ KEY_RIGHT,	Key::Right	},
	{ KEY_UP,	Key::Up		},
	{ KEY_DOWN,	Key::Down	},
	{ KEY_HOME,	Key::Home	},
	{ KEY_END,	Key::End	},
	{ KEY_IC,	Key::Insert	},
	{ KEY_DC,	Key::Delete	},
	{ KEY_BACKSPACE,Key::Backspace	},
	{ KEY_BKSPACE,	Key::Backspace	}
    };
    for (auto& km : c_keymap)
	if (km.ck == k)
	    return km.ek;
    return Event::key_t(k);
}

void TerminalScreen::TimerR_timer (PTimerR::fd_t)
{
    if (_windows.empty())
	return;
    auto wfocus = _windows.back();
    assert (wfocus);
    if (!*wfocus)
	return;
    for (int k; 0 < (k = wfocus->getch());) {
	if (k == KEY_RESIZE) {
	    _scrinfo.set_size (COLS, LINES);
	    for (auto w : _windows)
		w->Screen_get_info();
	} else
	    wfocus->on_event (Event (Event::Type::KeyDown, event_key_from_curses(k)));
    }
    if (!wfocus->flag (TerminalScreenWindow::f_Unused) && !isendwin())
	_uiinput.wait_read (STDIN_FILENO);
}

//}}}-------------------------------------------------------------------
//{{{ TerminalScreenWindow

TerminalScreenWindow::TerminalScreenWindow (const Msg::Link& l)
: Msger (l)
,_reply (l)
,_w()
,_viewport{}
,_caret{-1,-1}
,_winfo()
{
    TerminalScreen::instance().register_window (this);
}

TerminalScreenWindow::~TerminalScreenWindow (void)
{
    if (auto w = exchange(_w,nullptr); w)
	delwin(w);
    TerminalScreen::instance().unregister_window (this);
}

bool TerminalScreenWindow::dispatch (Msg& msg)
{
    return PScreen::dispatch (this, msg)
	|| Msger::dispatch (msg);
}

void TerminalScreenWindow::on_event (const Event& ev)
{
    _reply.event (ev);
}

void TerminalScreenWindow::Screen_close (void)
{
    set_unused (true);
}

//}}}-------------------------------------------------------------------
//{{{ Sizing and layout

void TerminalScreenWindow::Screen_get_info (void)
{
    _reply.screen_info (screen_info());
}

void TerminalScreenWindow::Screen_open (const WindowInfo& wi)
{
    _winfo = wi;
    TerminalScreen::instance().create_window (_winfo, _w);
    _reply.restate (_winfo);
}

//}}}-------------------------------------------------------------------
//{{{ Drawing operations

void TerminalScreenWindow::Screen_draw (const cmemlink& dl)
{
    clear();
    _caret.x = -1;
    Drawlist::dispatch (this, dl);
    if (_caret.x >= 0) {
	if (!flag (f_CaretOn)) {
	    leaveok (_w, false);
	    curs_set (1);
	    set_flag (f_CaretOn);
	}
	wmove (_w, _caret.y, _caret.x);
    } else if (flag (f_CaretOn)) {
	leaveok (_w, true);
	curs_set (0);
	set_flag (f_CaretOn, false);
    }
    refresh();
}

void TerminalScreenWindow::draw_color (IColor c)
{
    if (c == IColor::DefaultBold)
	wattron (_w, A_BOLD);
    else if (c == IColor::DefaultBackground)
	wattron (_w, A_REVERSE);
    else
	wattroff (_w, A_BOLD| A_REVERSE);
}

void TerminalScreenWindow::fill_color (IColor c)
{
    if (c == IColor::DefaultBold)
	wattron (_w, A_BLINK);
    else if (c == IColor::DefaultForeground)
	wattron (_w, A_REVERSE);
    else
	wattroff (_w, A_BLINK| A_REVERSE);
}

unsigned TerminalScreenWindow::clip_dx (unsigned dx) const
    { return min (dx, viewport().x + viewport().w - getcurx(_w)); }
unsigned TerminalScreenWindow::clip_dy (unsigned dx) const
    { return min (dx, viewport().y + viewport().h - getcury(_w)); }

void TerminalScreenWindow::move_to (coord_t x, coord_t y)
    { wmove (_w, viewport().y+y, viewport().x+x); }
void TerminalScreenWindow::move_by (coord_t dx, coord_t dy)
    { wmove (_w, dy+getcury(_w), dx+getcurx(_w)); }
void TerminalScreenWindow::addch (wchar_t c)
    { waddch (_w, c); }
void TerminalScreenWindow::noutrefresh (void)
    { wnoutrefresh (_w); }
void TerminalScreenWindow::refresh (void)
    { wrefresh (_w); }
int TerminalScreenWindow::getch (void)
    { return wgetch (_w); }
void TerminalScreenWindow::hline (unsigned n, wchar_t c)
    { whline (_w, c, clip_dx(n)); }
void TerminalScreenWindow::vline (unsigned n, wchar_t c)
    { wvline (_w, c, clip_dy(n)); }

void TerminalScreenWindow::box (dim_t w, dim_t h)
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

void TerminalScreenWindow::bar (dim_t w, dim_t h)
{
    for (dim_t y = 0; y < h; ++y) {
	hline (w, ' ');
	move_by (0, 1);
    }
    move_by (0, -h);
}

void TerminalScreenWindow::erase (void)
{
    draw_color (IColor::DefaultForeground);
    fill_color (IColor::DefaultBackground);
    move_to (viewport().pos());
    bar (viewport().w, viewport().h);
}

void TerminalScreenWindow::clear (void)
{
    _viewport = area();
    werase (_w);
}

void TerminalScreenWindow::panel (const Size& wh, PanelType t)
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

void TerminalScreenWindow::Draw_edit_text (const string& t, uint32_t cp, HAlign ha, VAlign va)
{
    _caret.x = getcurx(_w)+cp;
    _caret.y = getcury(_w);
    Draw_text (t, ha, va);
}

//}}}-------------------------------------------------------------------
//{{{ Drawlist dispatch

void TerminalScreenWindow::Draw_line (const Offset& o)
{
    if (o.dy)
	vline (o.dy, ACS_VLINE);
    else
	hline (o.dx, ACS_HLINE);
}

void TerminalScreenWindow::Draw_text (const string& t, HAlign ha, VAlign)
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
#endif // __has_include(<curses.h>)
