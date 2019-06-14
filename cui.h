// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#if __has_include(<curses.h>)
#include "app.h"
#include <curses.h>

//--- CursesWindow -----------------------------------------------------
namespace cwiclo {

class CursesWindow : public Msger {
public:
    using key_t	= int;
    enum { COLOR_DEFAULT = -1 };	// Curses transparent color
    enum {			// Keys that curses does not define
	KEY_ESCAPE = '\e',
	KEY_BKSPACE = '~'+1
    };
    enum { f_CaretOn = Msger::f_Last, f_Last };
public:
    explicit		CursesWindow (const Msg::Link& l);
			~CursesWindow (void) override;
    bool		dispatch (Msg& msg) override;
    void		on_msger_destroyed (mrid_t mid) override;
    inline virtual void	draw (void) const { wrefresh (_winput); }
    virtual void	on_key (key_t key);
    void		close (void);
    void		TimerR_timer (PTimerR::fd_t);
protected:
    //{{{ Ctrl ---------------------------------------------------------

    class Ctrl {
    public:
	enum { caret_is_visible = false };
	enum { f_focused, f_Last };
    public:
	constexpr		Ctrl (void)		:_w(),_flags() {}
				Ctrl (const Ctrl&) = delete;
				~Ctrl (void)		{ destroy(); }
	constexpr auto		w (void) const		{ return _w; }
	constexpr		operator WINDOW* (void)	const	{ return w(); }
	void			operator= (const Ctrl&) = delete;
	constexpr auto		operator-> (void) const	{ return w(); }
	constexpr u_short	maxx (void) const	{ return getmaxx(w()); }
	constexpr u_short	maxy (void) const	{ return getmaxy(w()); }
	constexpr u_short	begx (void) const	{ return getbegx(w()); }
	constexpr u_short	begy (void) const	{ return getbegy(w()); }
	constexpr u_short	curx (void) const	{ return getcurx(w()); }
	constexpr u_short	cury (void) const	{ return getcury(w()); }
	void			destroy (void)
				    { auto ow = exchange(_w,nullptr); if (ow) delwin(ow); }
	void			create (int l, int c, int y, int x);
	constexpr auto		flag (unsigned f) const			{ return get_bit(_flags,f); }
	constexpr void		set_flag (unsigned f, bool v = true)	{ set_bit(_flags,f,v); }
	constexpr auto		focused (void) const			{ return flag (f_focused); }
	constexpr void		transfer_focus_to (Ctrl& c)		{ set_flag (f_focused, false); c.set_flag (f_focused); }
    private:
	WINDOW*		_w;
	u_short		_flags;
    };
    //}}}---------------------------------------------------------------
    //{{{ Label

    class Label : public Ctrl {
    public:
	struct Size { uint16_t	x,y; };
    public:
	constexpr	Label (void) : Ctrl() {}
	static constexpr Size measure (const string_view& text) {
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
	void		draw (const char* text) const;
	void		draw (const string_view& text) const
			    { draw (text.c_str()); }
    };
    //}}}---------------------------------------------------------------
    //{{{ Button

    class Button : public Ctrl {
    public:
	constexpr explicit	Button (const char* t = "")	:Ctrl(),_text(t) {}
	void			create (int c, int y, int x)	{ Ctrl::create (1,c,y,x); }
	constexpr uint16_t	measure (void)		{ return __builtin_strlen(_text)+4; }
	void			draw (void) const;
	constexpr auto		text (void) const	{ return _text; }
	constexpr void		set_text (const char* t){ _text = t; }
    private:
	const char*	_text;
    };
    //}}}---------------------------------------------------------------
    //{{{ Listbox

    class Listbox : public Ctrl {
    public:
	using lcount_t	= u_short;
    public:
	constexpr	Listbox (void)		:Ctrl(),n(),sel(),top() {}
	constexpr void	clip_sel (void)		{ if (n && sel > n-1) sel = n-1; }
	constexpr void	set_n (lcount_t nn)	{ n = nn; clip_sel(); }
	void		on_key (key_t k);
	template <typename drawRow>
	inline void	draw (drawRow drf) const;
    public:
	lcount_t	n;
	lcount_t	sel;
	lcount_t	top;
    };
    //}}}---------------------------------------------------------------
    //{{{ Editbox

    class Editbox : public Ctrl {
    public:
	enum { caret_is_visible = true };
    public:
	constexpr	Editbox (void)			:Ctrl(),_cpos(),_fc(),_text() {}
	explicit	Editbox (const string& t)	:Ctrl(),_cpos(t.size()),_fc(),_text(t) {}
	void		create (int c, int y, int x);
	constexpr auto&	text (void) const		{ return _text; }
	void		set_text (const char* t)	{ _text = t; _cpos = _text.size(); _fc = 0; posclip(); }
	void		set_text (const string& t)	{ _text = t; _cpos = _text.size(); _fc = 0; posclip(); }
	void		on_key (key_t k);
	void		draw (void) const;
    private:
	void		posclip (void);
    private:
	u_short		_cpos;
	u_short		_fc;
	string		_text;
    };
    //}}}---------------------------------------------------------------
    //{{{ Focus rotation
private:
    void set_caret_visible (bool v = true)
	{ set_flag (f_CaretOn, v); curs_set(v); }
    template <typename F>
    inline void init_ctrl_focus_iter (F& f) {
	f.set_flag (Ctrl::f_focused);
	set_input_ctrl (f);
	if (F::caret_is_visible)
	    set_caret_visible();
    }
    template <typename F, typename H, typename... C>
    inline void init_ctrl_focus_iter (F& f, H& h, C&... c)
	{ if (!h.focused()) init_ctrl_focus_iter (f,c...); }
protected:
    template <typename H, typename... C>
    inline void init_ctrl_focus (H& h, C&... c)
	{ init_ctrl_focus_iter (h,h,c...); }
    template <typename F, typename T>
    inline void rotate_ctrl_focus (F& f, T& t) {
	f.transfer_focus_to (t);
	set_input_ctrl (t);
	if (F::caret_is_visible != T::caret_is_visible)
	    set_caret_visible (T::caret_is_visible);
    }
    //}}}---------------------------------------------------------------
protected:
    void		set_input_ctrl (Ctrl& c);
    inline virtual void	layout (void) { draw(); }
private:
    WINDOW*		_winput;
    PTimer		_uiinput;
    static mrid_t	s_focused;
    static uint8_t	s_nwins;
};

//{{{ Ctrls inlines ----------------------------------------------------

template <typename drawRow>
void CursesWindow::Listbox::draw (drawRow drf) const
{
    for (u_short y = 0, yend = min(n, top+maxy()); y < yend; ++y) {
	if (y >= top) {
	    wmove (w(), y-top, 0);
	    if (y == sel && focused()) {
		wattron (w(), A_REVERSE);
		whline (w(), ' ', maxx()-curx());
	    }
	    drf (w(), y);
	    if (y == sel)
		wattroff (w(), A_REVERSE);
	}
    }
}
//}}}

//--- MessageBox -------------------------------------------------------

//{{{ PMessageBox ------------------------------------------------------

class PMessageBox : public Proxy {
    DECLARE_INTERFACE (MessageBox, (ask,"qqs"))
public:
    enum class Answer : uint16_t { Cancel, Ok, Ignore, Yes = Ok, Retry = Ok, No = Ignore };
    enum class Type : uint16_t { Ok, OkCancel, YesNo, YesNoCancel, RetryCancelIgnore };
public:
    constexpr explicit	PMessageBox (mrid_t caller) : Proxy (caller) {}
    void		ask (const string& prompt, Type type = Type())
			    { send (m_ask(), type, uint16_t(0), prompt); }
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() != m_ask())
	    return false;
	auto is = msg.read();
	auto type = is.read<Type>();
	auto flags = is.read<uint16_t>();
	auto prompt = string_view_from_const_stream(is);
	o->MessageBox_ask (prompt, type, flags);
	return true;
    }
};

//}}}-------------------------------------------------------------------
//{{{ PMessageBoxR

class PMessageBoxR : public ProxyR {
    DECLARE_INTERFACE (MessageBoxR, (answer,"q"))
public:
    using Answer = PMessageBox::Answer;
public:
    explicit		PMessageBoxR (const Msg::Link& l) : ProxyR (l) {}
    void		reply (Answer answer) { send (m_answer(), answer); }
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() != m_answer())
	    return false;
	o->MessageBoxR_reply (msg.read().read<Answer>());
	return true;
    }
};

//}}}-------------------------------------------------------------------

class MessageBox : public CursesWindow {
    using Type = PMessageBox::Type;
    using Answer = PMessageBox::Answer;
public:
    explicit		MessageBox (const Msg::Link& l);
    bool		dispatch (Msg& msg) override;
    inline void		MessageBox_ask (const string_view& prompt, Type type, uint16_t flags);
    void		layout (void) override;
    void		draw (void) const override;
    void		on_key (key_t key) override;
private:
    void		done (Answer answer);
private:
    string		_prompt;
    Ctrl		_w;
    Label		_wmsg;
    Button		_wcancel;
    Button		_wok;
    Button		_wignore;
    Type		_type;
    Answer		_answer;
    PMessageBoxR	_reply;
};

} // namespace cwiclo
#endif // __has_include(<curses.h>)
