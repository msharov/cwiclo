// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "uidefs.h"

namespace cwiclo {
namespace ui {

class Drawlist {
protected:
    //{{{ Cmd enum -----------------------------------------------------
    enum class Cmd : uint8_t {
	Reset,
	Enable,
	Disable,
	Clear,
	MoveTo,
	MoveBy,
	Viewport,
	DrawColor,
	FillColor,
	Char,
	Text,
	Line,
	Box,
	Bar,
	CharBar,
	Panel,
	EditText,
	Last
    };
    struct CmdHeader {
	uint16_t	argsz;
	uint8_t		cmd;
	uint8_t		a1;
    };
    //}}}---------------------------------------------------------------
public:
    //{{{ GChar --------------------------------------------------------
    // Graphical characters for line drawing on text terminals.
    enum class GChar : uint16_t {
	ULCorner = 0xfdd0, LLCorner, URCorner, LRCorner,
	LeftT, RightT, TopT, BottomT,
	HLine, VLine, Plus, HLine1,
	HLine3, HLine7, HLine9, LeftArrow,
	RightArrow, UpArrow, DownArrow, Block,
	Board, Checkerboard, GreaterEqual, LessEqual,
	NotEqual, PlusMinus, Bullet, Degree,
	Diamond, Lantern, Pi, Sterling
    };
    //}}}---------------------------------------------------------------
    //{{{ Features for Cmd::Enable/Disable
    struct Feature {
	enum : uint8_t {
	    BoldText,
	    ItalicText,
	    UnderlineText,
	    BlinkText,
	    ReverseColors,
	    Last
	};
    };
    //}}}---------------------------------------------------------------
    //{{{ Writer
    template <typename Stm>
    class Writer {
    public:
	using stream_type	= Stm;
	enum {
	    is_reading = stream_type::is_reading,
	    is_sizing = stream_type::is_sizing,
	    is_writing = stream_type::is_writing
	};
    public:
	inline constexpr	Writer (void) = default;
	inline constexpr	Writer (stream_type&& stm)	: _stm(move(stm)) {}
	inline constexpr auto	size (void)			{ return _stm.size(); }
	inline constexpr auto	remaining (void)		{ return _stm.remaining(); }
	constexpr		Writer (const Writer& w) = delete;
	constexpr void		operator= (const Writer& w) = delete;
	//{{{2 command writer templates --------------------------------
    private:
	inline constexpr void	write_cmd_header (Cmd cmd, uint16_t argsz = 0, uint8_t a1 = 0) {
				    assert (_stm.aligned(4) && "Drawlist commands must be 4-aligned");
				    _stm << CmdHeader { argsz, uint8_t(cmd), a1 };
				}
    protected:
	inline constexpr void	write (Cmd cmd, uint8_t a1 = 0) { write_cmd_header (cmd, 0, a1); }
	template <typename... Arg>
	inline constexpr void	write (Cmd cmd, uint8_t a1, const Arg&... args) {
				    auto argsz = divide_ceil (variadic_stream_size (args...), 4);
				    assert (argsz <= UINT16_MAX && "Drawlist command size exceeded");
				    if (argsz > UINT16_MAX)
					return;
				    write_cmd_header (cmd, argsz, a1);
				    (_stm << ... << args);
				    assert (_stm.aligned(4) && "Drawlist command size must be a multiple of 4");
				}
	inline static constexpr auto pack_alignment_byte (HAlign ha, VAlign va)
				    { return uint8_t(uint8_t(ha)|uint8_t(va)<<2); }
	//}}}2----------------------------------------------------------
    protected:
	inline constexpr void	line (const Offset& d)		{ write (Cmd::Line, 0, d); }
	inline constexpr void	line (coord_t dx, coord_t dy)	{ line (Offset {dx,dy}); }
    public:
	inline constexpr void	reset (void)			{ write (Cmd::Reset); }
	inline constexpr void	enable (uint8_t feature)	{ write (Cmd::Enable, feature); }
	inline constexpr void	disable (uint8_t feature)	{ write (Cmd::Disable, feature); }
	inline constexpr void	clear (void)			{ write (Cmd::Clear); }
	inline constexpr void	move_to (const Point& pt)	{ write (Cmd::MoveTo, 0, pt); }
	inline constexpr void	move_to (coord_t x, coord_t y)	{ move_to (Point {x,y}); }
	inline constexpr void	move_by (const Offset& d)	{ write (Cmd::MoveBy, 0, d); }
	inline constexpr void	move_by (coord_t dx, coord_t dy){ move_by (Offset {dx,dy}); }
	inline constexpr void	viewport (const Rect& r)	{ write (Cmd::Viewport, 0, r); }
	inline constexpr void	draw_color (icolor_t c)		{ write (Cmd::DrawColor, c); }
	inline constexpr void	fill_color (icolor_t c)		{ write (Cmd::FillColor, c); }
	inline constexpr void	draw_char (wchar_t c, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
				    { write (Cmd::Char, pack_alignment_byte (ha,va), uint32_t(c)); }
	inline constexpr void	draw_char (GChar c, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
				    { draw_char (wchar_t(c), ha, va); }
	inline constexpr void	text (const string& s, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
				    { write (Cmd::Text, pack_alignment_byte (ha,va), s); }
	inline constexpr void	text (const string_view& s, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
				    { text (s.str(), ha, va); }
	inline constexpr void	text (const char* s, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
				    { text (string_view(s), ha, va); }
	inline constexpr void	text (const char* s, unsigned n, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
				    { text (string_view(s,n), ha, va); }
	inline constexpr void	text (char c, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
				    { text (string_view(&c,1), ha, va); }
	inline constexpr void	edit_text (const string& s, uint32_t cp, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
				    { write (Cmd::EditText, pack_alignment_byte (ha,va), cp, s); }
	inline constexpr void	edit_text (const string_view& s, uint32_t cp, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
				    { edit_text (s.str(), cp, ha, va); }
	inline constexpr void	edit_text (const char* s, uint32_t cp, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
				    { edit_text (string_view(s), cp, ha, va); }
	inline constexpr void	edit_text (const char* s, unsigned n, uint32_t cp, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
				    { edit_text (string_view(s,n), cp, ha, va); }
	inline constexpr void	edit_text (char c, uint32_t cp, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
				    { edit_text (string_view(&c,1), cp, ha, va); }
	inline constexpr void	hline (coord_t dx)		{ line (dx, 0); }
	inline constexpr void	vline (coord_t dy)		{ line (0, dy); }
	inline constexpr void	box (const Size& wh)		{ write (Cmd::Box, 0, wh); }
	inline constexpr void	box (dim_t w, dim_t h)		{ box (Size(w,h)); }
	inline constexpr void	box (const Rect& r)		{ move_to (r.pos()); box (r.size()); }
	inline constexpr void	bar (const Size& wh)		{ write (Cmd::Bar, 0, wh); }
	inline constexpr void	bar (dim_t w, dim_t h)		{ bar (Size(w,h)); }
	inline constexpr void	bar (const Rect& r)		{ move_to (r.pos()); bar (r.size()); }
	inline constexpr void	char_bar (const Size& wh, wchar_t c)	{ write (Cmd::CharBar, 0, wh, uint32_t(c)); }
	inline constexpr void	char_bar (dim_t w, dim_t h, wchar_t c)	{ char_bar (Size(w,h), c); }
	inline constexpr void	char_bar (const Rect& r, wchar_t c)	{ move_to (r.pos()); char_bar (r.size(), c); }
	inline constexpr void	panel (const Size& wh, PanelType t = PanelType::Raised)	{ write (Cmd::Panel, uint8_t(t), wh); }
	inline constexpr void	panel (dim_t w, dim_t h, PanelType t=PanelType::Raised)	{ panel (Size(w,h), t); }
	inline constexpr void	panel (const Rect& r, PanelType t = PanelType::Raised)	{ move_to (r.pos()); panel (r.size(), t); }
    private:
	stream_type	_stm;
    };
    //}}}---------------------------------------------------------------
    //{{{ validate
protected:
    template <typename VFunc>
    inline static auto validate_with_func (istream dls, VFunc vfunc) {
	streamsize r = 0;
	while (dls.remaining() >= 4) {
	    auto h = dls.read<CmdHeader>();
	    auto argbytes = 4u*h.argsz;
	    if (argbytes > dls.remaining())
		break;	// invalid command
	    if (argbytes != vfunc (h.cmd, istream (dls.ptr(), argbytes)))
		break;
	    r += 4+argbytes;
	    dls.skip (argbytes);
	}
	return r;
    }
    inline static auto validate_cmd (uint8_t cmd, istream args) {
	static constexpr const char c_sigs[] =
	    ""				// Reset
	    "\0"			// Enable
	    "\0"			// Disable
	    "\0"			// Clear
	    "\0" SIGNATURE_ui_Point	// MoveTo
	    "\0" SIGNATURE_ui_Offset	// MoveBy
	    "\0" SIGNATURE_ui_Rect	// Viewport
	    "\0" "u"			// DrawColor
	    "\0" "u"			// FillColor
	    "\0" "u"			// Char
	    "\0" "s"			// Text
	    "\0" SIGNATURE_ui_Offset	// Line
	    "\0" SIGNATURE_ui_Size	// Box
	    "\0" SIGNATURE_ui_Size	// Bar
	    "\0" SIGNATURE_ui_Size "u"	// CharBar
	    "\0" SIGNATURE_ui_Size	// Panel
	    "\0" "us"			// EditText
	;
	static_assert (size_t(Cmd::Last) == zstr::nstrs(c_sigs), "c_sigs must contain signatures for each Cmd");
	if (cmd >= uint8_t(Cmd::Last))
	    return args.remaining();	// unknown commands are not validated internally
	return Msg::validate_signature (args, zstr::at (cmd, c_sigs));
    }
public:
    // Use validate_with_func to generate a combined validator
    static auto validate (istream dls)
	{ return validate_with_func (dls, validate_cmd); }
    //}}}---------------------------------------------------------------
    //{{{ dispatch
protected:
    //
    // dispatch_with_func is the general command reading template, iterating over
    // all commands in the given drawlist and calling dfunc on each one.
    //
    template <typename Impl, typename DFunc>
    inline static constexpr void dispatch_with_func (Impl* impl, istream dls, DFunc dfunc) {
	while (dls.remaining() >= 4) {
	    auto h = dls.read<CmdHeader>();
	    auto argbytes = 4u*h.argsz;
	    if (argbytes > dls.remaining())
		break;	// invalid command
	    dfunc (impl, h, istream (dls.ptr(), argbytes));
	    dls.skip (argbytes);
	}
    }
    // dispatch_cmd unmarshals and dispatches each command
    template <typename Impl>
    inline static constexpr void dispatch_cmd (Impl* impl, const CmdHeader& h, istream argstm) {
	assert (argstm.remaining() == validate_cmd (h.cmd, argstm) && "validate the drawlist before dispatching");
	switch (Cmd(h.cmd)) {
	    case Cmd::Reset:	impl->Draw_reset(); break;
	    case Cmd::Enable:	impl->Draw_enable (h.a1); break;
	    case Cmd::Disable:	impl->Draw_disable (h.a1); break;
	    case Cmd::Clear:	impl->Draw_clear(); break;
	    case Cmd::MoveTo:	impl->Draw_move_to (argstm.read<Point>()); break;
	    case Cmd::MoveBy:	impl->Draw_move_by (argstm.read<Offset>()); break;
	    case Cmd::Viewport:	impl->Draw_viewport (argstm.read<Rect>()); break;
	    case Cmd::DrawColor: impl->Draw_draw_color (h.a1); break;
	    case Cmd::FillColor: impl->Draw_fill_color (h.a1); break;
	    case Cmd::Char:	impl->Draw_char (argstm.read<uint32_t>(), HAlign(h.a1&3), VAlign(h.a1>>2)); break;
	    case Cmd::Text:	impl->Draw_text (argstm.read<string_view>(), HAlign(h.a1&3), VAlign(h.a1>>2)); break;
	    case Cmd::Line:	impl->Draw_line (argstm.read<Offset>()); break;
	    case Cmd::Box:	impl->Draw_box (argstm.read<Size>()); break;
	    case Cmd::Bar:	impl->Draw_bar (argstm.read<Size>()); break;
	    case Cmd::CharBar:	{ decltype(auto) sz = argstm.read<Size>();
				  impl->Draw_char_bar (move(sz), argstm.read<uint32_t>()); break; }
	    case Cmd::Panel:	impl->Draw_panel (argstm.read<Size>(), PanelType(h.a1)); break;
	    case Cmd::EditText: { auto cp = argstm.read<uint32_t>();
				  impl->Draw_edit_text (argstm.read<string_view>(), cp, HAlign(h.a1&3), VAlign(h.a1>>2));
				break; }
	    default:		break;
	};
    }
public:
    // Use dispatch_with_func to generate a combined dispatcher
    template <typename Impl>
    inline static constexpr void dispatch (Impl* impl, istream dls)
	{ dispatch_with_func (impl, dls, dispatch_cmd<Impl>); }
    //}}}---------------------------------------------------------------
};

class DrawlistGraphic : public Drawlist {
protected:
    //{{{ Cmd enum -----------------------------------------------------
    enum class Cmd : uint8_t {
	DefineColor = uint8_t(Drawlist::Cmd::Last),
	Palette,
	Palette3,
	Last
    };
    //}}}---------------------------------------------------------------
public:
    //{{{ Writer
    template <typename Stm>
    class Writer : public Drawlist::Writer<Stm> {
    public:
	inline constexpr	Writer (void) = default;
	inline constexpr	Writer (Stm&& stm) : Drawlist::Writer<Stm> (move(stm)) {}
    private:
	inline constexpr void	write (Cmd cmd, uint8_t a1 = 0) { Drawlist::Writer<Stm>::write (Drawlist::Cmd(cmd), 0, a1); }
	template <typename... Arg>
	inline constexpr void	write (Cmd cmd, uint8_t a1, const Arg&... args) { Drawlist::Writer<Stm>::write (Drawlist::Cmd(cmd), a1, forward<Arg>(args)...); }
    public:
				using Drawlist::Writer<Stm>::line;
	inline constexpr void	define_color (icolor_t c, color_t rgb)	{ write (Cmd::DefineColor, c, rgb); }
	inline constexpr void	palette (const vector<color_t>& pal, icolor_t f = 0) {
				    assert (pal.size()+f <= numeric_limits<colray_t>::max() && "Palette out of range");
				    write (Cmd::Palette, f, pal);
				}
	template <unsigned N>
	inline constexpr void	palette (const color_t (&pal)[N], icolor_t f = 0) {
				    assert (N+f <= numeric_limits<colray_t>::max() && "Palette out of range");
				    write (Cmd::Palette, f, pal);
				}
	inline constexpr void	palette3 (const vector<colray_t>& pal, icolor_t f = 0) {
				    assert (!(pal.size()%3) && "palette3 takes a vector of RGB triplets, one per color");
				    assert (pal.size()/3+f <= numeric_limits<colray_t>::max() && "Palette out of range");
				    write (Cmd::Palette3, f, pal);
				}
	template <unsigned N>
	inline constexpr void	palette3 (const colray_t (&pal)[N], icolor_t f = 0) {
				    static_assert (!(N%3), "palette3 takes an array of RGB triplets, one per color");
				    assert (N/3+f <= numeric_limits<colray_t>::max() && "Palette out of range");
				    write (Cmd::Palette3, f, pal);
				}
    };
    //}}}---------------------------------------------------------------
    //{{{ validate
protected:
    inline static auto validate_cmd (uint8_t cmd, istream args) {
	static constexpr const char c_sigs[] =
	    "u"		// DefineColor
	    "\0au"	// Palette
	    "\0ay"	// Palette3
	;
	static_assert (size_t(Cmd::Last)-size_t(Drawlist::Cmd::Last) == zstr::nstrs(c_sigs), "c_sigs must contain signatures for each Cmd");
	cmd -= uint8_t(Drawlist::Cmd::Last);
	if (cmd >= uint8_t(Cmd::Last))
	    return args.remaining();	// unknown commands are not validated internally
	return Msg::validate_signature (args, zstr::at (cmd, c_sigs));
    }
public:
    // Use validate_with_func to generate a combined validator
    static auto validate (istream dls)
	{ return validate_with_func (dls, validate_cmd); }
    //}}}---------------------------------------------------------------
    //{{{ dispatch
protected:
    template <typename Impl>
    inline static constexpr void dispatch_cmd (Impl* impl, const CmdHeader& h, istream argstm) {
	assert (argstm.remaining() == validate_cmd (h.cmd, argstm) && "validate the drawlist before dispatching");
	switch (Cmd(h.cmd)) {
	    case Cmd::DefineColor:	impl->Draw_set_color (h.a1, argstm.read<color_t>()); break;
	    case Cmd::Palette:		impl->Draw_palette (h.a1, argstm.read<vector_view<color_t>>()); break;
	    case Cmd::Palette3:		impl->Draw_palette3 (h.a1, argstm.read<vector_view<colray_t>>()); break;
	    default:			Drawlist::dispatch_cmd (impl, h, argstm); break;
	}
    }
public:
    template <typename Impl>
    inline static constexpr void dispatch (Impl* impl, istream dls)
	{ dispatch_with_func (impl, dls, dispatch_cmd<Impl>); }
    //}}}---------------------------------------------------------------
};

} // namespace ui
} // namespace cwiclo
