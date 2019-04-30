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
    explicit		CursesWindow (const Msg::Link& l) noexcept;
			~CursesWindow (void) noexcept override;
    bool		Dispatch (Msg& msg) noexcept override;
    void		OnMsgerDestroyed (mrid_t mid) noexcept override;
    inline virtual void	Draw (void) const noexcept { wrefresh (_winput); }
    virtual void	OnKey (key_t key) noexcept;
    void		Close (void) noexcept;
    void		TimerR_Timer (int) noexcept;
protected:
    //{{{ Ctrl ---------------------------------------------------------

    class Ctrl {
    public:
	enum { caret_is_visible = false };
	enum { f_Focused, f_Last };
    public:
			Ctrl (void)		:_w(),_flags() {}
			Ctrl (const Ctrl&) = delete;
			~Ctrl (void) noexcept	{ Destroy(); }
	auto		W (void) const		{ return _w; }
			operator WINDOW* (void)	const	{ return W(); }
	void		operator= (const Ctrl&) = delete;
	auto		operator-> (void) const	{ return W(); }
	inline u_short	maxx (void) const	{ return getmaxx(W()); }
	inline u_short	maxy (void) const	{ return getmaxy(W()); }
	inline u_short	begx (void) const	{ return getbegx(W()); }
	inline u_short	begy (void) const	{ return getbegy(W()); }
	inline u_short	curx (void) const	{ return getcurx(W()); }
	inline u_short	cury (void) const	{ return getcury(W()); }
	void		Destroy (void)
			    { auto ow = exchange(_w,nullptr); if (ow) delwin(ow); }
	void		Create (int l, int c, int y, int x) noexcept;
	auto		Flag (unsigned f) const			{ return GetBit(_flags,f); }
	void		SetFlag (unsigned f, bool v = true)	{ SetBit(_flags,f,v); }
	auto		Focused (void) const			{ return Flag (f_Focused); }
	void		TransferFocusTo (Ctrl& c)		{ SetFlag (f_Focused, false); c.SetFlag (f_Focused); }
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
			Label (void)		:Ctrl() {}
	static Size	Measure (const string_view& text) noexcept;
	void		Draw (const char* text) const noexcept;
	void		Draw (const string_view& text) const noexcept
			    { Draw (text.c_str()); }
    };
    //}}}---------------------------------------------------------------
    //{{{ Button

    class Button : public Ctrl {
    public:
	explicit	Button (const char* t = "")	:Ctrl(),_text(t) {}
	void		Create (int c, int y, int x)	{ Ctrl::Create (1,c,y,x); }
	uint16_t	Measure (void) noexcept	{ return strlen(_text)+4; }
	void		Draw (void) const noexcept;
	auto		Text (void) const	{ return _text; }
	void		SetText (const char* t)	{ _text = t; }
    private:
	const char*	_text;
    };
    //}}}---------------------------------------------------------------
    //{{{ Listbox

    class Listbox : public Ctrl {
    public:
	using lcount_t	= u_short;
    public:
			Listbox (void)		:Ctrl(),n(),sel(),top() {}
	inline void	ClipSel (void)		{ if (n && sel > n-1) sel = n-1; }
	inline void	SetN (lcount_t nn)	{ n = nn; ClipSel(); }
	void		OnKey (key_t k) noexcept;
	template <typename DrawRow>
	inline void	Draw (DrawRow drf) const noexcept;
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
			Editbox (void)			:Ctrl(),_cpos(),_fc(),_text() {}
	explicit	Editbox (const string& t)	:Ctrl(),_cpos(t.size()),_fc(),_text(t) {}
	void		Create (int c, int y, int x) noexcept;
	auto&		Text (void) const		{ return _text; }
	void		SetText (const char* t)		{ _text = t; _cpos = _text.size(); _fc = 0; Posclip(); }
	void		SetText (const string& t)	{ _text = t; _cpos = _text.size(); _fc = 0; Posclip(); }
	void		OnKey (key_t k) noexcept;
	void		Draw (void) const noexcept;
    private:
	void		Posclip (void) noexcept;
    private:
	u_short		_cpos;
	u_short		_fc;
	string		_text;
    };
    //}}}---------------------------------------------------------------
    //{{{ Focus rotation
private:
    void SetCaretVisible (bool v = true)
	{ SetFlag (f_CaretOn, v); curs_set(v); }
    template <typename F>
    inline void InitCtrlFocusIter (F& f) {
	f.SetFlag (Ctrl::f_Focused);
	SetInputCtrl (f);
	if (F::caret_is_visible)
	    SetCaretVisible();
    }
    template <typename F, typename H, typename... C>
    inline void InitCtrlFocusIter (F& f, H& h, C&... c)
	{ if (!h.Focused()) InitCtrlFocusIter (f,c...); }
protected:
    template <typename H, typename... C>
    inline void InitCtrlFocus (H& h, C&... c)
	{ InitCtrlFocusIter (h,h,c...); }
    template <typename F, typename T>
    inline void RotateCtrlFocus (F& f, T& t) {
	f.TransferFocusTo (t);
	SetInputCtrl (t);
	if (F::caret_is_visible != T::caret_is_visible)
	    SetCaretVisible (T::caret_is_visible);
    }
    //}}}---------------------------------------------------------------
protected:
    void		SetInputCtrl (Ctrl& c) noexcept;
    inline virtual void	Layout (void) noexcept { Draw(); }
private:
    WINDOW*		_winput;
    PTimer		_uiinput;
    static mrid_t	s_focused;
    static uint8_t	s_nWindows;
};

//{{{ Ctrls inlines ----------------------------------------------------

template <typename DrawRow>
void CursesWindow::Listbox::Draw (DrawRow drf) const noexcept
{
    for (u_short y = 0, yend = min(n, top+maxy()); y < yend; ++y) {
	if (y >= top) {
	    if (y == sel && Focused())
		wattron (W(), A_REVERSE);
	    drf (W(), y);
	    if (y == sel) {
		whline (W(), ' ', maxx()-curx());
		wattroff (W(), A_REVERSE);
	    }
	    wmove (W(), cury()+1, 0);
	}
    }
}
//}}}

//--- MessageBox -------------------------------------------------------

//{{{ PMessageBox ------------------------------------------------------

class PMessageBox : public Proxy {
    DECLARE_INTERFACE (MessageBox, (Ask,"qqs"))
public:
    enum class Answer : uint16_t { Cancel, Ok, Ignore, Yes = Ok, Retry = Ok, No = Ignore };
    enum class Type : uint16_t { Ok, OkCancel, YesNo, YesNoCancel, RetryCancelIgnore };
public:
    explicit		PMessageBox (mrid_t caller) : Proxy (caller) {}
    void		Ask (const string& prompt, Type type = Type())
			    { Send (M_Ask(), type, uint16_t(0), prompt); }
    template <typename O>
    inline static bool	Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() != M_Ask())
	    return false;
	auto is = msg.Read();
	auto type = is.read<Type>();
	auto flags = is.read<uint16_t>();
	auto prompt = string_view_from_const_stream(is);
	o->MessageBox_Ask (prompt, type, flags);
	return true;
    }
};

//}}}-------------------------------------------------------------------
//{{{ PMessageBoxR

class PMessageBoxR : public ProxyR {
    DECLARE_INTERFACE (MessageBoxR, (Answer,"q"))
public:
    using Answer = PMessageBox::Answer;
public:
    explicit		PMessageBoxR (const Msg::Link& l) : ProxyR (l) {}
    void		Reply (Answer answer) { Send (M_Answer(), answer); }
    template <typename O>
    inline static bool	Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() != M_Answer())
	    return false;
	o->MessageBoxR_Reply (msg.Read().read<Answer>());
	return true;
    }
};

//}}}-------------------------------------------------------------------

class MessageBox : public CursesWindow {
    using Type = PMessageBox::Type;
    using Answer = PMessageBox::Answer;
public:
    explicit		MessageBox (const Msg::Link& l) noexcept;
    bool		Dispatch (Msg& msg) noexcept override;
    inline void		MessageBox_Ask (const string_view& prompt, Type type, uint16_t flags) noexcept;
    void		Layout (void) noexcept override;
    void		Draw (void) const noexcept override;
    void		OnKey (key_t key) noexcept override;
private:
    void		Done (Answer answer) noexcept;
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
