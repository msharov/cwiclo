// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#if __has_include(<curses.h>)
#include "cui.h"
#include <signal.h>
#include <ctype.h>

namespace cwiclo {

//{{{ CursesWindow -----------------------------------------------------

mrid_t CursesWindow::s_focused = 0;
uint8_t CursesWindow::s_nWindows = 0;

//----------------------------------------------------------------------

CursesWindow::CursesWindow (const Msg::Link& l) noexcept
: Msger(l)
,_winput()
,_uiinput (l.dest)
{
    if (!s_nWindows++) {
	if (!initscr()) {
	    Error ("unable to initialize terminal output");
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

CursesWindow::~CursesWindow (void) noexcept
{
    if (!--s_nWindows) {
	flushinp();
	endwin();
    }
}

void CursesWindow::SetInputCtrl (Ctrl& c) noexcept
{
    _winput = c.W();
    if (_winput) {
	keypad (_winput, true);
	meta (_winput, true);
	nodelay (_winput, true);
	s_focused = MsgerId();
	TimerR_Timer (STDIN_FILENO);
    }
}

void CursesWindow::OnMsgerDestroyed (mrid_t mid) noexcept
{
    Msger::OnMsgerDestroyed (mid);
    // Check if a child modal dialog closed
    if (mid == s_focused && _winput) {
	curs_set (Flag (f_CaretOn));
	s_focused = MsgerId();
	TimerR_Timer (STDIN_FILENO);
    }
}

void CursesWindow::OnKey (key_t k) noexcept
{
    if (k == KEY_RESIZE)
	Layout();
}

void CursesWindow::Close (void) noexcept
{
    SetFlag (f_Unused);
    _uiinput.Stop();
}

void CursesWindow::TimerR_Timer (int) noexcept
{
    if (MsgerId() != s_focused || !_winput)
	return;	// a modal dialog is active
    for (key_t k; 0 < (k = wgetch (_winput));)
	OnKey (k);
    if (!Flag(f_Unused) && !isendwin()) {
	Draw();
	_uiinput.WaitRead (STDIN_FILENO);
    }
}

bool CursesWindow::Dispatch (Msg& msg) noexcept
{
    return PTimerR::Dispatch(this, msg)
	|| Msger::Dispatch(msg);
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Ctrl

void CursesWindow::Ctrl::Create (int l, int c, int y, int x) noexcept
{
    auto ow = exchange(_w,newwin(l,c,y,x));
    if (ow)
	delwin(ow);
    leaveok (*this, true);
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Label

auto CursesWindow::Label::Measure (const string_view& text) noexcept -> Size
{
    Size sz = {};
    for (auto l = text.begin(), textend = text.end(); l < textend;) {
	auto lend = text.find ('\n', l);
	if (!lend)
	    lend = textend;
	sz.x = max (sz.x, lend-l+1);
	++sz.y;
	l = lend+1;
    }
    return sz;
}

void CursesWindow::Label::Draw (const char* text) const noexcept
{
    werase (*this);
    waddstr (*this, text);
    wnoutrefresh (*this);
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Button

void CursesWindow::Button::Draw (void) const noexcept
{
    werase (*this);
    if (Focused())
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

void CursesWindow::Listbox::OnKey (key_t k) noexcept
{
    if ((k == 'k' || k == KEY_UP) && sel)
	--sel;
    else if ((k == 'j' || k == KEY_DOWN) && sel+1 < n)
	++sel;
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Editbox

void CursesWindow::Editbox::Create (int c, int y, int x) noexcept
{
    Ctrl::Create (1,c,y,x);
    leaveok (*this, false);
    wbkgdset (*this, ' '|A_UNDERLINE);
    Posclip();
}

void CursesWindow::Editbox::Posclip (void) noexcept
{
    if (_cpos < _fc) {	// cursor past left edge
	_fc = _cpos;
	if (_fc > 0)
	    --_fc;	// adjust for clip indicator
    }
    if (W()) {
	if (_fc+maxx() < _cpos+2)	// cursor past right edge +2 for > indicator
	    _fc = _cpos+2-maxx();
	while (_fc && _fc-1u+maxx() > _text.size())	// if text fits in box, no scroll
	    --_fc;
    }
}

void CursesWindow::Editbox::OnKey (key_t k) noexcept
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
	_text.insert (_text.iat(_cpos++), k);
    Posclip();
}

void CursesWindow::Editbox::Draw (void) const noexcept
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

MessageBox::MessageBox (const Msg::Link& l) noexcept
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

bool MessageBox::Dispatch (Msg& msg) noexcept
{
    return PMessageBox::Dispatch (this, msg)
	|| CursesWindow::Dispatch (msg);
}

void MessageBox::MessageBox_Ask (const string_view& prompt, Type type, uint16_t) noexcept
{
    _prompt = prompt;
    _type = type;
    if (_type == Type::YesNo) {
	_wok.SetText ("Yes");
	_wcancel.SetText ("No");
    } else if (_type == Type::YesNoCancel) {
	_wok.SetText ("Yes");
	_wignore.SetText ("No");
    } else if (_type == Type::RetryCancelIgnore)
	_wok.SetText ("Retry");
    Layout();
}

void MessageBox::Layout (void) noexcept
{
    // Measure message
    auto lsz = Label::Measure (_prompt);
    lsz.y = min (lsz.y, LINES-4u);
    lsz.x = min (lsz.x, COLS-4u);

    // Measure button strip width
    auto okw = _wok.Measure(), cancelw = _wcancel.Measure(), ignorew = _wignore.Measure();
    auto bw = okw;
    if (_type != Type::Ok)
	bw += cancelw;
    if (_type == Type::YesNoCancel || _type == Type::RetryCancelIgnore)
	bw += ignorew;

    // Create main dialog window
    auto wy = lsz.y+4;
    auto wx = max(lsz.x,bw)+4;
    _w.Create (wy, wx, (LINES-wy)/2u, (COLS-wx)/2u);
    // The message is just inside, centered
    _wmsg.Create (lsz.y, lsz.x, _w.begy()+1, _w.begx()+(_w.maxx()-lsz.x)/2u);

    // Create button row, centered under message
    auto bbeg = _w.begx()+(_w.maxx()-bw)/2u;
    auto bry = _w.begy()+_w.maxy()-2;
    _wok.Create (okw, bry, bbeg);
    bbeg += okw;
    if (_type == Type::YesNoCancel || _type == Type::RetryCancelIgnore) {
	_wignore.Create (ignorew, bry, bbeg);
	bbeg += ignorew;
    }

    // Initialize focus
    if (_type == Type::Ok)
	InitCtrlFocus (_wok);
    else {
	_wcancel.Create (cancelw, bry, bbeg);
	InitCtrlFocus (_wcancel, _wok, _wignore);
    }
    CursesWindow::Layout();
}

void MessageBox::Draw (void) const noexcept
{
    werase (_w);
    box (_w, 0, 0);
    wnoutrefresh (_w);

    _wmsg.Draw (_prompt);

    _wok.Draw();
    if (_type != Type::Ok) {
	_wcancel.Draw();
	if (_type == Type::YesNoCancel || _type == Type::RetryCancelIgnore)
	    _wignore.Draw();
    }
    CursesWindow::Draw();
}

void MessageBox::Done (Answer answer) noexcept
{
    _reply.Reply (answer);
    Close();
}

void MessageBox::OnKey (key_t key) noexcept
{
    if (key == 'y' || key == 'r')
	Done (Answer::Ok);
    else if (key == 'n' || key == 'i')
	Done (Answer::No);
    else if (key == KEY_ESCAPE || key == 'c')
	Done (Answer::Cancel);

    if (_wcancel.Focused()) {
	if (key == '\t')
	    RotateCtrlFocus (_wcancel, _wok);
	else if (key == '\n')
	    Done (Answer::Cancel);
    } else if (_wignore.Focused()) {
	if (key == '\t')
	    RotateCtrlFocus (_wignore, _wcancel);
	else if (key == '\n')
	    Done (Answer::Ignore);
    } else {
	if (key == '\t') {
	    if (_type == Type::OkCancel || _type == Type::YesNo)
		RotateCtrlFocus (_wok, _wcancel);
	    else if (_type == Type::YesNoCancel || _type == Type::RetryCancelIgnore)
		RotateCtrlFocus (_wok, _wignore);
	} else if (key == '\n')
	    Done (Answer::Ok);
    }
    CursesWindow::OnKey (key);
}

//}}}-------------------------------------------------------------------

} // namespace cwiclo
#endif // __has_include(<curses.h>)
