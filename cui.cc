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
,_focused (wid_None)
,_widgets()
{
    if (s_nwins++)
	return;
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

CursesWindow::~CursesWindow (void)
{
    if (!--s_nwins) {
	flushinp();
	endwin();
    }
}

auto CursesWindow::widget_by_id (widgetid_t id) const -> const Widget*
{
    if (id)
	for (auto& w : _widgets)
	    if (w->widget_id() == id)
		return w.get();
    return nullptr;
}

auto CursesWindow::create_widget (const Widget::Layout& lay) -> Widget*
{
    Widget* w;
    Msg::Link l { msger_id(), msger_id() };
    switch (lay.type) {
	default:			w = nullptr;			break;
	case Widget::Type::Label:	w = new Label (l,lay);		break;
	case Widget::Type::Button:	w = new Button (l,lay);		break;
	case Widget::Type::Listbox:	w = new Listbox (l,lay);	break;
	case Widget::Type::Editbox:	w = new Editbox (l,lay);	break;
	case Widget::Type::HSplitter:	w = new HSplitter (l,lay);	break;
	case Widget::Type::VSplitter:	w = new VSplitter (l,lay);	break;
	case Widget::Type::GroupFrame:	w = new GroupFrame (l,lay);	break;
	case Widget::Type::StatusLine:	w = new StatusLine (l,lay);	break;
    }
    if (w)
	_widgets.emplace_back (w);
    return w;
}

void CursesWindow::create_widgets (const Widget::Layout* l, unsigned sz)
{
    for (; sz; ++l, --sz)
	create_widget (*l);
}

void CursesWindow::focus_widget (widgetid_t id)
{
    auto newf = widget_by_id (id);
    if (!newf || !newf->flag (Widget::f_CanFocus) || !newf->w())
	return;
    auto oldf = focused_widget();
    if (oldf)
	oldf->set_flag (Widget::f_Focused, false);
    newf->set_flag (Widget::f_Focused);
    _focused = newf->widget_id();
    _winput = newf->w();
    bool newcaret = newf->flag (Widget::f_HasCaret);
    if (!oldf || oldf->flag (Widget::f_HasCaret) != newcaret) {
	set_flag (f_CaretOn, newcaret);
	curs_set (newcaret);
    }
    s_focused = msger_id();
    TimerR_timer (STDIN_FILENO);
}

void CursesWindow::focus_next (void)
{
    widgetid_t oid = focused_widget_id(), nid = wid_Last, fid = wid_Last;
    for (auto& w : _widgets) {
	if (!w->widget_id() || !w->flag (Widget::f_CanFocus) || !w->w())
	    continue;			// Focusable widgets must set flag, have id, and be laid out
	fid = min (fid, w->widget_id());	// widget with lowest id for wraparound
	if (w->widget_id() > oid && w->widget_id() < nid)
	    nid = w->widget_id();		// pick the closest next id
    }
    if (nid == wid_Last)
	nid = fid;	// If nothing found, wrap around to first
    focus_widget (nid);
}

void CursesWindow::focus_prev (void)
{
    widgetid_t oid = focused_widget_id(), nid = wid_None, lid = wid_None;
    for (auto& w : _widgets) {
	if (!w->widget_id() || !w->flag (Widget::f_CanFocus) || !w->w())
	    continue;			// Focusable widgets must set flag, have id, and be laid out
	lid = max (lid, w->widget_id());	// widget with highest id for wraparound
	if (w->widget_id() < oid && w->widget_id() > nid)
	    nid = w->widget_id();		// pick the closest previous id
    }
    if (!nid)
	nid = lid;	// If nothing found, wrap around to last
    focus_widget (nid);
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

void CursesWindow::layout (void)
{
    if (!focused_widget_id())
	focus_next();	// initialize focus if needed
    draw();
}

void CursesWindow::draw (void) const
{
    for (auto& w : _widgets)
	if (w->w())
	    w->draw();
    if (_winput)
	wnoutrefresh (_winput);
    doupdate();
}

void CursesWindow::on_event (const Event& ev)
{
    // KeyDown events go only to the focused widget
    if (ev.type() == Event::Type::KeyDown) {
	// Key events that the focused widget does not use are forwarded
	// back here with the source widget id set to that widget.
	auto focusw = focused_widget();
	if (focusw && ev.src() != focusw->widget_id())
	    focusw->on_event (ev);
	else
	    on_key (ev.key());	// key events unused by widgets are processed in the window handler
    } else
	for (auto& w : _widgets)
	    if (ev.src() != w->widget_id())
		w->on_event (ev);
}

void CursesWindow::on_key (key_t k)
{
    if (k == KEY_RESIZE)
	layout();
    else if (k == '\t' || k == KEY_RIGHT || k == KEY_DOWN)
	focus_next();
    else if (k == KEY_LEFT || k == KEY_UP)
	focus_prev();
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
    for (int k; 0 < (k = wgetch (_winput));)
	PWidget_event (Event (Event::Type::KeyDown, key_t(k)));
    if (!flag (f_Unused) && !isendwin()) {
	draw();
	_uiinput.wait_read (STDIN_FILENO);
    }
}

bool CursesWindow::dispatch (Msg& msg)
{
    return PTimerR::dispatch (this, msg)
	|| PWidgetR::dispatch (this, msg)
	|| Msger::dispatch (msg);
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Widget

CursesWindow::Widget::Widget (const Msg::Link& l, const Layout& lay)
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

void CursesWindow::Widget::resize (int l, int c, int y, int x)
{
    auto ow = exchange (_w, newwin (l,c,y,x));
    if (ow)
	delwin (ow);
    set_area (x, y, c, l);
    leaveok (w(), true);
    keypad (w(), true);
    meta (w(), true);
    nodelay (w(), true);
    on_resize();
}

auto CursesWindow::Widget::measure_text (const string_view& text) -> Size
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

void CursesWindow::Widget::report_modified (void)
{
    _reply.modified (widget_id(), text());
}

void CursesWindow::Widget::on_set_text (void)
{
    set_size_hints (measure_text (text()));
}

void CursesWindow::Widget::on_event (const Event& ev)
{
    if (ev.type() == Event::Type::KeyDown)
	on_key (ev.key());
}

void CursesWindow::Widget::on_key (key_t k)
{
    create_event (Event (Event::Type::KeyDown, k, widget_id()));
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Label

void CursesWindow::Label::on_draw (void) const
{
    Widget::on_draw();
    waddstr (w(), text().c_str());
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Button

void CursesWindow::Button::on_draw (void) const
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

void CursesWindow::Button::on_set_text (void)
{
    Widget::on_set_text();
    set_size_hints (strlen("[ ") + size_hints().w + strlen (" ]"), size_hints().h);
}

//}}}-------------------------------------------------------------------
//{{{ CursesWindow::Listbox

void CursesWindow::Listbox::on_set_text (void)
{
    set_n (zstr::nstrs (text().c_str(), text().size()));
    clip_sel();
}

void CursesWindow::Listbox::on_draw (void) const
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

void CursesWindow::Listbox::on_key (key_t k)
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
//{{{ CursesWindow::Editbox

void CursesWindow::Editbox::on_resize (void)
{
    Widget::on_resize();
    leaveok (w(), false);
    wbkgdset (w(), ' '|A_UNDERLINE);
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
	while (_fc && _fc-1u+maxx() > text().size())	// if text fits in box, no scroll
	    --_fc;
    }
}

void CursesWindow::Editbox::on_set_text (void)
{
    _cpos = text().size();
    _fc = 0;
    posclip();
}

void CursesWindow::Editbox::on_key (key_t k)
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

void CursesWindow::Editbox::on_draw (void) const
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
//{{{ MessageBox

DEFINE_INTERFACE (MessageBox)
DEFINE_INTERFACE (MessageBoxR)

//----------------------------------------------------------------------

MessageBox::MessageBox (const Msg::Link& l)
: CursesWindow(l)
,_prompt()
,_type()
,_answer()
,_reply(l)
{
    create_widgets (c_layout);
    set_widget_text (wid_Cancel, "Cancel");
    set_widget_text (wid_OK, "Ok");
    set_widget_text (wid_Ignore, "Ignore");
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
	set_widget_text (wid_OK, "Yes");
	set_widget_text (wid_Cancel, "No");
    } else if (_type == Type::YesNoCancel) {
	set_widget_text (wid_OK, "Yes");
	set_widget_text (wid_Ignore, "No");
    } else if (_type == Type::RetryCancelIgnore)
	set_widget_text (wid_OK, "Retry");
    layout();
}

void MessageBox::layout (void)
{
    // measure message
    auto wmsg = widget_by_id (wid_Message);
    wmsg->set_text (_prompt);
    auto lsz = wmsg->size_hints();
    lsz.h = min (lsz.h, LINES-4u);
    lsz.w = min (lsz.w, COLS-4u);

    // measure button strip width
    auto wok = widget_by_id (wid_OK);
    auto wcancel = widget_by_id (wid_Cancel);
    auto wignore = widget_by_id (wid_Ignore);
    auto okw = wok->size_hints().w, cancelw = wcancel->size_hints().w, ignorew = wignore->size_hints().w;
    auto bw = okw;
    if (_type != Type::Ok)
	bw += cancelw;
    if (_type == Type::YesNoCancel || _type == Type::RetryCancelIgnore)
	bw += ignorew;

    // create main dialog window
    auto wy = lsz.h+4;
    auto wx = max(lsz.w,bw)+4;
    auto wframe = widget_by_id (wid_Frame);
    wframe->resize (wy, wx, (LINES-wy)/2u, (COLS-wx)/2u);
    // The message is just inside, centered
    wmsg->resize (lsz.h, lsz.w, wframe->begy()+1, wframe->begx()+(wframe->maxx()-lsz.w)/2u);

    // create button row, centered under message
    auto bbeg = wframe->begx()+(wframe->maxx()-bw)/2u;
    auto bry = wframe->begy()+wframe->maxy()-2;
    wok->resize (1, okw, bry, bbeg);
    bbeg += okw;
    if (_type == Type::YesNoCancel || _type == Type::RetryCancelIgnore) {
	wignore->resize (1, ignorew, bry, bbeg);
	bbeg += ignorew;
    }
    if (_type != Type::Ok)
	wcancel->resize (1, cancelw, bry, bbeg);
    CursesWindow::layout();
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
    else if (key == '\n') {
	if (focused_widget_id() == wid_Cancel)
	    done (Answer::Cancel);
	else if (focused_widget_id() == wid_Ignore)
	    done (Answer::Ignore);
	else if (focused_widget_id() == wid_OK)
	    done (Answer::Ok);
	else
	    CursesWindow::on_key (key);
    } else
	CursesWindow::on_key (key);
}

//}}}-------------------------------------------------------------------

} // namespace cwiclo
#endif // __has_include(<curses.h>)
