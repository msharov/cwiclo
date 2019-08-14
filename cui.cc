// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#if __has_include(<curses.h>)
#include "cui.h"
#include <signal.h>
#include <ctype.h>

namespace cwiclo {

//{{{ CursesWindow -----------------------------------------------------

mrid_t CursesWindow::s_focused = 0;
uint8_t CursesWindow::s_nwins = 0;

//----------------------------------------------------------------------

CursesWindow::CursesWindow (const Msg::Link& l)
: Msger(l)
,_winput()
,_uiinput (l.dest)
{
    if (!s_nwins++) {
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
}

CursesWindow::~CursesWindow (void)
{
    if (!--s_nwins) {
	flushinp();
	endwin();
    }
}

void CursesWindow::set_input_ctrl (Ctrl& c)
{
    _winput = c.w();
    if (_winput) {
	keypad (_winput, true);
	meta (_winput, true);
	nodelay (_winput, true);
	s_focused = msger_id();
	TimerR_timer (STDIN_FILENO);
    }
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

void CursesWindow::on_key (key_t k)
{
    if (k == KEY_RESIZE)
	layout();
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
    for (key_t k; 0 < (k = wgetch (_winput));)
	on_key (k);
    if (!flag (f_Unused) && !isendwin()) {
	draw();
	_uiinput.wait_read (STDIN_FILENO);
    }
}

bool CursesWindow::dispatch (Msg& msg)
{
    return PTimerR::dispatch (this, msg)
	|| Msger::dispatch (msg);
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Ctrl

void CursesWindow::Ctrl::create (int l, int c, int y, int x)
{
    auto ow = exchange (_w, newwin (l,c,y,x));
    if (ow)
	delwin (ow);
    leaveok (*this, true);
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Label

void CursesWindow::Label::draw (const char* text) const
{
    werase (*this);
    waddstr (*this, text);
    wnoutrefresh (*this);
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Button

void CursesWindow::Button::draw (void) const
{
    werase (*this);
    if (focused())
	wattron (*this, A_REVERSE);
    wattron (*this, A_BOLD);
    waddstr (*this, "[ ");
    waddch (*this, _text[0]);
    wattroff (*this, A_BOLD);
    waddstr (*this, &_text[1]);
    wattron (*this, A_BOLD);
    waddstr (*this, " ]");
    wattroff (*this, A_BOLD| A_REVERSE);
    wnoutrefresh (*this);
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Listbox

void CursesWindow::Listbox::on_key (key_t k)
{
    if ((k == 'k' || k == KEY_UP) && sel)
	--sel;
    else if ((k == 'j' || k == KEY_DOWN) && sel+1 < n)
	++sel;
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Editbox

void CursesWindow::Editbox::create (int c, int y, int x)
{
    Ctrl::create (1,c,y,x);
    leaveok (*this, false);
    wbkgdset (*this, ' '|A_UNDERLINE);
    posclip();
}

void CursesWindow::Editbox::posclip (void)
{
    if (_cpos < _fc) {	// cursor past left edge
	_fc = _cpos;
	if (_fc > 0)
	    --_fc;	// adjust for clip indicator
    }
    if (w()) {
	if (_fc+maxx() < _cpos+2)	// cursor past right edge +2 for > indicator
	    _fc = _cpos+2-maxx();
	while (_fc && _fc-1u+maxx() > _text.size())	// if text fits in box, no scroll
	    --_fc;
    }
}

void CursesWindow::Editbox::on_key (key_t k)
{
    if (k == KEY_LEFT && _cpos)
	--_cpos;
    else if (k == KEY_RIGHT && _cpos < _text.size())
	++_cpos;
    else if (k == KEY_HOME)
	_cpos = 0;
    else if (k == KEY_END)
	_cpos = _text.size();
    else if ((k == KEY_BKSPACE || k == KEY_BACKSPACE) && _cpos)
	_text.erase (_text.iat(--_cpos));
    else if (k == KEY_DC && _cpos < _text.size())
	_text.erase (_text.iat(_cpos));
    else if (isprint(k))
	_text.insert (_text.iat(_cpos++), char(k));
    posclip();
}

void CursesWindow::Editbox::draw (void) const
{
    mvwhline (*this, 0, 0, ' ', maxx());
    waddstr (*this, _text.iat(_fc));
    if (_fc)
	mvwaddch (*this, 0, 0, '<');
    if (_fc+maxx() <= _text.size())
	mvwaddch (*this, 0, maxx()-1, '>');
    wmove (*this, 0, _cpos-_fc);
    wnoutrefresh (*this);
}

//}}}-------------------------------------------------------------------
//{{{ MessageBox

DEFINE_INTERFACE (MessageBox)
DEFINE_INTERFACE (MessageBoxR)

//----------------------------------------------------------------------

MessageBox::MessageBox (const Msg::Link& l)
: CursesWindow(l)
,_prompt()
,_w()
,_wmsg()
,_wcancel ("Cancel")
,_wok ("Ok")
,_wignore ("Ignore")
,_type()
,_answer()
,_reply(l)
{
}

bool MessageBox::dispatch (Msg& msg)
{
    return PMessageBox::dispatch (this, msg)
	|| CursesWindow::dispatch (msg);
}

void MessageBox::MessageBox_ask (const string_view& prompt, Type type, uint16_t)
{
    _prompt = prompt;
    _type = type;
    if (_type == Type::YesNo) {
	_wok.set_text ("Yes");
	_wcancel.set_text ("No");
    } else if (_type == Type::YesNoCancel) {
	_wok.set_text ("Yes");
	_wignore.set_text ("No");
    } else if (_type == Type::RetryCancelIgnore)
	_wok.set_text ("Retry");
    layout();
}

void MessageBox::layout (void)
{
    // measure message
    auto lsz = Label::measure (_prompt);
    lsz.y = min (lsz.y, LINES-4u);
    lsz.x = min (lsz.x, COLS-4u);

    // measure button strip width
    auto okw = _wok.measure(), cancelw = _wcancel.measure(), ignorew = _wignore.measure();
    auto bw = okw;
    if (_type != Type::Ok)
	bw += cancelw;
    if (_type == Type::YesNoCancel || _type == Type::RetryCancelIgnore)
	bw += ignorew;

    // create main dialog window
    auto wy = lsz.y+4;
    auto wx = max(lsz.x,bw)+4;
    _w.create (wy, wx, (LINES-wy)/2u, (COLS-wx)/2u);
    // The message is just inside, centered
    _wmsg.create (lsz.y, lsz.x, _w.begy()+1, _w.begx()+(_w.maxx()-lsz.x)/2u);

    // create button row, centered under message
    auto bbeg = _w.begx()+(_w.maxx()-bw)/2u;
    auto bry = _w.begy()+_w.maxy()-2;
    _wok.create (okw, bry, bbeg);
    bbeg += okw;
    if (_type == Type::YesNoCancel || _type == Type::RetryCancelIgnore) {
	_wignore.create (ignorew, bry, bbeg);
	bbeg += ignorew;
    }

    // Initialize focus
    if (_type == Type::Ok)
	init_ctrl_focus (_wok);
    else {
	_wcancel.create (cancelw, bry, bbeg);
	init_ctrl_focus (_wcancel, _wok, _wignore);
    }
    CursesWindow::layout();
}

void MessageBox::draw (void) const
{
    werase (_w);
    box (_w, 0, 0);
    wnoutrefresh (_w);

    _wmsg.draw (_prompt);

    _wok.draw();
    if (_type != Type::Ok) {
	_wcancel.draw();
	if (_type == Type::YesNoCancel || _type == Type::RetryCancelIgnore)
	    _wignore.draw();
    }
    CursesWindow::draw();
}

void MessageBox::done (Answer answer)
{
    _reply.reply (answer);
    close();
}

void MessageBox::on_key (key_t key)
{
    if (key == 'y' || key == 'r')
	done (Answer::Ok);
    else if (key == 'n' || key == 'i')
	done (Answer::No);
    else if (key == KEY_ESCAPE || key == 'c')
	done (Answer::Cancel);

    if (_wcancel.focused()) {
	if (key == '\t')
	    rotate_ctrl_focus (_wcancel, _wok);
	else if (key == '\n')
	    done (Answer::Cancel);
    } else if (_wignore.focused()) {
	if (key == '\t')
	    rotate_ctrl_focus (_wignore, _wcancel);
	else if (key == '\n')
	    done (Answer::Ignore);
    } else {
	if (key == '\t') {
	    if (_type == Type::OkCancel || _type == Type::YesNo)
		rotate_ctrl_focus (_wok, _wcancel);
	    else if (_type == Type::YesNoCancel || _type == Type::RetryCancelIgnore)
		rotate_ctrl_focus (_wok, _wignore);
	} else if (key == '\n')
	    done (Answer::Ok);
    }
    CursesWindow::on_key (key);
}

//}}}-------------------------------------------------------------------

} // namespace cwiclo
#endif // __has_include(<curses.h>)
