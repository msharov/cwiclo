// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "msg.h"

namespace cwiclo {
namespace ui {

//{{{ Graphics related types -------------------------------------------

using coord_t	= int16_t;
using dim_t	= make_unsigned_t<coord_t>;
using colray_t	= uint8_t;
using icolor_t	= uint8_t;
using color_t	= uint32_t;

//{{{2 Offset

struct Offset {
    coord_t	dx;
    coord_t	dy;
public:
    constexpr		Offset (void)			: dx(0),dy(0) {}
    constexpr		Offset (coord_t nx, coord_t ny)	: dx(nx),dy(ny) {}
    constexpr bool	operator== (const Offset& o) const { return dx == o.dx && dy == o.dy; }
    constexpr bool	operator!= (const Offset& o) const { return !operator==(o); }
    constexpr auto	operator+ (const Offset& o) const { return Offset (dx+o.dx, dy+o.dy); }
    constexpr auto	operator- (const Offset& o) const { return Offset (dx-o.dx, dy-o.dy); }
    constexpr auto&	operator+= (const Offset& o)	{ dx += o.dx; dy += o.dy; return *this; }
    constexpr auto&	operator-= (const Offset& o)	{ dx -= o.dx; dy -= o.dy; return *this; }
};
#define SIGNATURE_ui_Offset	"(nn)"
//}}}2
//{{{2 Size

struct Size {
    dim_t	w;
    dim_t	h;
public:
    constexpr		Size (void)			: w(0),h(0) {}
    constexpr		Size (coord_t nw, coord_t nh)	: w(nw),h(nh) {}
    constexpr bool	operator== (const Size& o) const{ return w == o.w && h == o.h; }
    constexpr bool	operator!= (const Size& o) const{ return !operator==(o); }
    constexpr auto	operator+ (const Size& o) const	{ return Size (w+o.w, h+o.h); }
    constexpr auto	operator- (const Size& o) const	{ return Size (w-o.w, h-o.h); }
    constexpr auto&	operator+= (const Size& o)	{ w += o.w; h += o.h; return *this; }
    constexpr auto&	operator-= (const Size& o)	{ w -= o.w; h -= o.h; return *this; }
    constexpr auto	as_offset (void) const		{ return Offset(w,h); }
};
#define SIGNATURE_ui_Size	"(qq)"

//}}}2
//{{{2 Point

struct Point {
    coord_t	x;
    coord_t	y;
public:
    constexpr		Point (void)			: x(0),y(0) {}
    constexpr		Point (coord_t nx, coord_t ny)	: x(nx),y(ny) {}
    constexpr bool	operator== (const Point& p) const { return x == p.x && y == p.y; }
    constexpr bool	operator!= (const Point& p) const { return !operator==(p); }
    constexpr auto	operator+ (const Offset& o) const { return Point (x+o.dx, y+o.dy); }
    constexpr auto	operator+ (const Size& o) const   { return Point (x+o.w, y+o.h); }
    constexpr auto	operator+ (const Point& o) const  { return Point (x+o.x, y+o.y); }
    constexpr auto	operator- (const Offset& o) const { return Point (x-o.dx, y-o.dy); }
    constexpr auto	operator- (const Size& o) const   { return Point (x-o.w, y-o.h); }
    constexpr auto	operator- (const Point& p) const  { return Offset (x-p.x, y-p.y); }
    constexpr auto&	operator+= (const Offset& o)	{ x += o.dx; y += o.dy; return *this; }
    constexpr auto&	operator+= (const Size& o)	{ x += o.w; y += o.h; return *this; }
    constexpr auto&	operator+= (const Point& o)	{ x += o.x; y += o.y; return *this; }
    constexpr auto&	operator-= (const Offset& o)	{ x -= o.dx; y -= o.dy; return *this; }
    constexpr auto&	operator-= (const Size& o)	{ x -= o.w; y -= o.h; return *this; }
    constexpr auto&	operator-= (const Point& o)	{ x -= o.x; y -= o.y; return *this; }
    constexpr auto	as_offset (void) const		{ return Offset(x,y); }
};
#define SIGNATURE_ui_Point	"(nn)"

//}}}2
//{{{2 Rect

struct Rect {
    coord_t	x;
    coord_t	y;
    dim_t	w;
    dim_t	h;
public:
    constexpr		Rect (void)			: x(0),y(0),w(0),h(0) {}
    constexpr		Rect (coord_t nx, coord_t ny, dim_t nw, dim_t nh) :x(nx),y(ny),w(nw),h(nh) {}
    constexpr		Rect (const Point& p, const Size& s) : Rect(p.x,p.y,s.w,s.h) {}
    explicit constexpr	Rect (const Size& s)		: Rect(0,0,s.w,s.h) {}
			struct Composite { Point xy; Size wh; };
    constexpr auto&	pos (void) const		{ return pointer_cast<Composite>(this)->xy; }
    constexpr auto&	size (void) const		{ return pointer_cast<Composite>(this)->wh; }
    constexpr bool	empty (void) const		{ return !(w && h); }
    constexpr bool	operator== (const Rect& r) const{ return pos() == r.pos() && size() == r.size(); }
    constexpr bool	operator!= (const Rect& r) const{ return !operator==(r); }
    constexpr bool	contains (coord_t px, coord_t py) const	{ return dim_t(px-x) < w && dim_t(py-y) < h; }
    constexpr bool	contains (const Point& p) const	{ return contains (p.x, p.y); }
    constexpr bool	contains (const Rect& r) const	{ return contains (r.pos()) && contains (r.pos()+r.size()); }
    constexpr void	move_to (const Point& p)	{ x = p.x; y = p.y; }
    constexpr void	move_by (const Offset& o)	{ x += o.dx; y += o.dy; }
    constexpr void	resize (const Size& s)		{ w = s.w; h = s.h; }
    constexpr void	assign (const Point& p, const Size& s)	{ move_to(p); resize(s); }
    [[nodiscard]] constexpr Rect clip (const Rect& r) const {
			    Rect cr;
			    cr.x = min (max (r.x, x), x+w);
			    cr.y = min (max (r.y, y), y+h);
			    cr.w = min (max (r.x+r.w, cr.x), x+w) - cr.x;
			    cr.h = min (max (r.y+r.h, cr.y), y+h) - cr.y;
			    return cr;
			}
};
#define SIGNATURE_ui_Rect	"(nnqq)"

//}}}2

enum class HAlign : uint8_t { Left, Center, Right };
enum class VAlign : uint8_t { Top, Center, Bottom };

enum class ScreenType : uint8_t { Text, Graphics, OpenGL, HTML, Printer };
enum class MSAA : uint8_t { OFF, X2, X4, X8, X16, MAX = X16 };

//----------------------------------------------------------------------

// Various types of custom-drawn UI elements
enum class PanelType : uint8_t {
    Raised, Sunken, Listbox, Editbox,
    Selection, Button, PressedButton, StatusBar
};

//----------------------------------------------------------------------

inline static constexpr color_t RGBA (colray_t r, colray_t g, colray_t b, colray_t a = numeric_limits<colray_t>::max())
    { return (color_t(a)<<24)|(color_t(b)<<16)|(color_t(g)<<8)|r; }
inline static constexpr color_t RGBA (color_t c)
    { return bswap(c); }
inline static constexpr color_t RGB (colray_t r, colray_t g, colray_t b)
    { return RGBA(r,g,b,numeric_limits<colray_t>::max()); }
inline static constexpr color_t RGB (color_t c)
    { return RGBA((c<<8)|0xff); }

//----------------------------------------------------------------------

// Color names for the standard 256 color xterm palette
struct IColor {
    enum : icolor_t {
	Black, Red, Green, Brown, Blue, Magenta, Cyan, Gray,
	DarkGray, LightRed, LightGreen, Yellow,
	LightBlue, LightMagenta, LightCyan, White,
	Default,
	Gray0 = 232, Gray08, Gray1, Gray2, Gray28, Gray3,
	Gray4, Gray48, Gray5, Gray6, Gray68, Gray7,
	Gray8, Gray88, Gray9, GrayA, GrayA8, GrayB,
	GrayC, GrayC8, GrayD, GrayE, GrayE8, GrayF
    };
};

//}}}-------------------------------------------------------------------
//{{{ Widget layout types

using widgetid_t = uint16_t;
enum : widgetid_t {
    wid_None,
    wid_First,
    wid_Last = numeric_limits<widgetid_t>::max()
};

class WidgetLayout {
public:
    enum class Type : uint8_t {
	None,
	HBox,
	VBox,
	Label,
	Button,
	Listbox,
	Editbox,
	HSplitter,
	VSplitter,
	GroupFrame,
	StatusLine
    };
public:
    constexpr		WidgetLayout (uint8_t l, Type t, widgetid_t id = wid_None, HAlign ha = HAlign::Left, VAlign va = VAlign::Top)
			    : _level(l),_halign(uint8_t(ha)),_valign(uint8_t(va)),_type(t),_id(id) {}
    constexpr		WidgetLayout (uint8_t l, Type t, HAlign ha, VAlign va = VAlign::Top)
			    : WidgetLayout (l,t,wid_None,ha,va) {}
    constexpr		WidgetLayout (uint8_t l, Type t, VAlign va)
			    : WidgetLayout (l,t,HAlign::Left,va) {}
    constexpr auto	level (void) const	{ return _level; }
    constexpr auto	type (void) const	{ return _type; }
    constexpr auto	id (void) const		{ return _id; }
    constexpr auto	halign (void) const	{ return HAlign(_halign); }
    constexpr auto	valign (void) const	{ return VAlign(_valign); }
private:
    uint8_t		_level:4;
    uint8_t		_halign:2;
    uint8_t		_valign:2;
    Type		_type;
    widgetid_t		_id;
};
#define SIGNATURE_ui_WidgetLayout	"(yyq)"

//{{{2 WL_ macros ---------------------------------------------------------
#define WL_L(l,tn,...)	::cwiclo::ui::WidgetLayout (l, ::cwiclo::ui::WidgetLayout::Type::tn, ## __VA_ARGS__)
#define WL_(tn,...)				WL_L( 1,tn, ## __VA_ARGS__)
#define WL___(tn,...)				WL_L( 2,tn, ## __VA_ARGS__)
#define WL_____(tn,...)				WL_L( 3,tn, ## __VA_ARGS__)
#define WL_______(tn,...)			WL_L( 4,tn, ## __VA_ARGS__)
#define WL_________(tn,...)			WL_L( 5,tn, ## __VA_ARGS__)
#define WL___________(tn,...)			WL_L( 6,tn, ## __VA_ARGS__)
#define WL_____________(tn,...)			WL_L( 7,tn, ## __VA_ARGS__)
#define WL_______________(tn,...)		WL_L( 8,tn, ## __VA_ARGS__)
#define WL_________________(tn,...)		WL_L( 9,tn, ## __VA_ARGS__)
#define WL___________________(tn,...)		WL_L(10,tn, ## __VA_ARGS__)
#define WL_____________________(tn,...)		WL_L(11,tn, ## __VA_ARGS__)
#define WL_______________________(tn,...)	WL_L(12,tn, ## __VA_ARGS__)
#define WL_________________________(tn,...)	WL_L(13,tn, ## __VA_ARGS__)
#define WL___________________________(tn,...)	WL_L(14,tn, ## __VA_ARGS__)
#define WL_____________________________(tn,...)	WL_L(15,tn, ## __VA_ARGS__)
//}}}2---------------------------------------------------------------------

//}}}-------------------------------------------------------------------
//{{{ Event

class Event {
public:
    using key_t	= char32_t;
    //{{{2 Type
    enum class Type : uint8_t {
	None,
	// User input
	KeyDown,
	KeyUp,
	ButtonDown,
	ButtonUp,
	Motion,
	Crossing,
	Clipboard,
	// Window control events
	Destroy,
	Close,
	Ping,
	VSync,
	Focus,
	Visibility
    };
    //}}}2
public:
    constexpr		Event (void)
			    :_type(Type::None),_mods(0),_src(wid_None),_key(0) {}
    explicit constexpr	Event (Type t, key_t k = 0, uint8_t mods = 0, widgetid_t src = wid_None)
			    :_type(t),_mods(mods),_src(src),_key(k) {}
    constexpr		Event (Type t, const Point& pt, uint8_t mods = 0, widgetid_t src = wid_None)
			    :_type(t),_mods(mods),_src(src),_pt(pt) {}
    constexpr		Event (Type t, const Size& sz, uint8_t mods = 0, widgetid_t src = wid_None)
			    :_type(t),_mods(mods),_src(src),_sz(sz) {}
    constexpr auto	src (void) const	{ return _src; }
    constexpr auto	type (void) const	{ return _type; }
    constexpr auto	mods (void) const	{ return _mods; }
    constexpr auto&	loc (void) const	{ return _pt; }
    constexpr auto&	size (void) const	{ return _sz; }
    constexpr auto	key (void) const	{ return _key; }
private:
    Type	_type;
    uint8_t	_mods;
    widgetid_t	_src;
    union {
	key_t	_key;
	Point	_pt;
	Size	_sz;
    };
};
#define SIGNATURE_ui_Event "(yyqu)"

//{{{2 KMod ------------------------------------------------------------

struct KMod {
    enum : Event::key_t { FirstBit = bits_in_type<Event::key_t>::value-8 };
    static constexpr const Event::key_t Shift	= bit_mask<Event::key_t>(FirstBit+0);
    static constexpr const Event::key_t Ctrl	= Shift<<1;
    static constexpr const Event::key_t Alt	= Ctrl<<1;
    static constexpr const Event::key_t Banner	= Alt<<1;
    static constexpr const Event::key_t Left	= Banner<<1;
    static constexpr const Event::key_t Middle	= Left<1;
    static constexpr const Event::key_t Right	= Middle<<1;
    static constexpr const Event::key_t Mask	= Shift-1;
};

//}}}2------------------------------------------------------------------
//{{{2 Key

struct Key {
    static constexpr const Event::key_t Mask = ~KMod::Mask;
    enum : Event::key_t {
	Null, Shift, PageUp, End, Pause,
	Search, Mute, Play, Backspace, Tab,
	Enter, Forward, PageDown, Home, CapsLock,
	F1, F2, F3, F4, F5,
	F6, F7, F8, F9, F10,
	F11, F12, Escape, Print, Paste,
	Save, Open, Space,
	Delete = '~'+1,
	Options, History, Break, Refresh, Favorites,
	Down, Copy, Cut, Center, Help,
	Back, Right, Left, Up, Alt,
	NumLock, SysReq, VolumeUp, Redo, ScrollLock,
	Undo, Mail, ZoomIn, ZoomOut, New,
	Wheel, Insert, Ctrl, Stop, Banner,
	VolumeDown, Menu
    };
};

//}}}2------------------------------------------------------------------
//{{{2 MouseButton

struct MouseButton {
    static constexpr const Event::key_t Mask = Key::Mask;
    enum : Event::key_t {
	None, Left, Middle, Right,
	WheelUp, WheelDown, WheelLeft, WheelRight
    };
};

//}}}2------------------------------------------------------------------
//{{{2 Visibility

enum class Visibility : uint8_t {
    Unobscured,
    PartiallyObscured,
    FullyObscured
};

//}}}2------------------------------------------------------------------
//{{{2 ClipboardOp

enum class ClipboardOp : uint8_t {
    Rejected,
    Accepted,
    Read,
    Cleared
};

//}}}2
//}}}-------------------------------------------------------------------
//{{{ Cursor

enum class Cursor : uint8_t {
    X, arrow, based_arrow_down, based_arrow_up, boat,
    bogosity, bottom_left_corner, bottom_right_corner, bottom_side, bottom_tee,
    box_spiral, center_ptr, circle, clock, coffee_mug,
    cross, cross_reverse, crosshair, diamond_cross, dot,
    dotbox, double_arrow, draft_large, draft_small, draped_box,
    exchange, fleur, gobbler, gumby, hand1,
    hand2, heart, icon, iron_cross, left_ptr,
    left_side, left_tee, leftbutton, ll_angle, lr_angle,
    man, middlebutton, mouse, pencil, pirate,
    plus, question_arrow, right_ptr, right_side, right_tee,
    rightbutton, rtl_logo, sailboat, sb_down_arrow, sb_h_double_arrow,
    sb_left_arrow, sb_right_arrow, sb_up_arrow, sb_v_double_arrow, shuttle,
    sizing, spider, spraycan, star, target,
    tcross, top_left_arrow, top_left_corner, top_right_corner, top_side,
    top_tee, trek, ul_angle, umbrella, ur_angle,
    watch, xterm, hidden
};

//}}}-------------------------------------------------------------------
//{{{ ScreenInfo

class ScreenInfo {
public:
    inline constexpr		ScreenInfo (void)
				    :_scrsz{},_physz{},_type(ScreenType::Text)
				    ,_depth(8),_gapi(0),_msaa(MSAA::OFF) {}
    inline constexpr		ScreenInfo (const Size& ssz, ScreenType st, uint8_t d, uint8_t gav = 0, MSAA aa = MSAA::OFF, const Size& phy = {})
				    :_scrsz{ssz},_physz{phy},_type(st),_depth(d),_gapi(gav),_msaa(aa) {}
    inline constexpr auto&	size (void) const		{ return _scrsz; }
    inline constexpr void	set_size (const Size& sz)	{ _scrsz = sz; }
    inline constexpr void	set_size (dim_t w, dim_t h)	{ _scrsz.w = w; _scrsz.h = h; }
    inline constexpr auto&	physical_size (void) const	{ return _physz; }
    inline constexpr auto	type (void) const		{ return _type; }
    inline constexpr auto	depth (void) const		{ return _depth; }
    inline constexpr void	set_depth (uint8_t d)		{ _depth = d; }
    inline constexpr auto	gapi_version (void) const	{ return _gapi; }
    inline constexpr auto	msaa (void) const		{ return _msaa; }
private:
    Size	_scrsz;
    Size	_physz;
    ScreenType	_type;
    uint8_t	_depth;
    uint8_t	_gapi;
    MSAA	_msaa;
};
#define SIGNATURE_ui_ScreenInfo	"(" SIGNATURE_ui_Size SIGNATURE_ui_Size "yyyy)"

//}}}-------------------------------------------------------------------
//{{{ WindowInfo

class WindowInfo {
public:
    //{{{2 Type
    enum class Type : uint8_t {
	Normal,
	Desktop,
	Dock,
	Dialog,
	Toolbar,
	Utility,
	Menu,
	PopupMenu,
	DropdownMenu,
	ComboMenu,
	Notification,
	Tooltip,
	Splash,
	Dragged,
	Embedded
    };
    //}}}2
    //{{{2 State
    enum class State : uint8_t {
	Normal,
	MaximizedX,
	MaximizedY,
	Maximized,
	Hidden,
	Fullscreen,
	Gamescreen	// Like fullscreen, but possibly change resolution to fit
    };
    //}}}2
    //{{{2 Flag
    enum class Flag : uint8_t {
	Focused,
	Modal,
	Attention,
	Sticky,
	NotOnTaskbar,
	NotOnPager,
	Above,
	Below
    };
    //}}}2
private:
    //{{{2 Type ranges
    enum class TypeRangeFirst : uint8_t {
	Parented	= uint8_t(Type::Dialog),
	Decoless	= uint8_t(Type::PopupMenu),
	PopupMenu	= uint8_t(Type::PopupMenu)
    };
    enum class TypeRangeLast : uint8_t {
	Parented	= uint8_t(Type::Splash),
	Decoless	= uint8_t(Type::Dragged),
	PopupMenu	= uint8_t(Type::ComboMenu)
    };
    static constexpr bool	in_type_range (Type t, TypeRangeFirst f, TypeRangeLast l)
				    { return uint8_t(t) >= uint8_t(f) && uint8_t(t) <= uint8_t(l); }
    //}}}2
public:
    inline constexpr		WindowInfo (void)
				    :_area{},_parent(),_type (Type::Normal)
				    ,_state (State::Normal),_cursor (Cursor::left_ptr)
				    ,_flags(),_gapi(),_msaa (MSAA::OFF) {}
    inline constexpr		WindowInfo (Type t, const Rect& area, extid_t parent = 0,
					State st = State::Normal, Cursor cursor = Cursor::left_ptr,
					uint8_t gapi = 0, MSAA aa = MSAA::OFF)
				    :_area(area),_parent(parent),_type (t)
				    ,_state (st),_cursor (cursor),_flags()
				    ,_gapi(gapi),_msaa (aa) {}
    inline constexpr auto&	area (void) const		{ return _area; }
    inline constexpr void	set_area (const Rect& a)	{ _area = a; }
    inline constexpr auto	parent (void) const		{ return _parent; }
    inline constexpr auto	type (void) const		{ return _type; }
    inline constexpr bool	is_parented (void) const	{ return in_type_range (type(), TypeRangeFirst::Parented, TypeRangeLast::Parented); }
    inline constexpr bool	is_decoless (void) const	{ return in_type_range (type(), TypeRangeFirst::Decoless, TypeRangeLast::Decoless); }
    inline constexpr bool	is_popup (void) const		{ return in_type_range (type(), TypeRangeFirst::PopupMenu, TypeRangeLast::PopupMenu); }
    inline constexpr auto	state (void) const		{ return _state; }
    inline constexpr void	set_state (State s)		{ _state = s; }
    inline constexpr auto	cursor (void) const		{ return _cursor; }
    inline constexpr void	set_cursor (Cursor c)		{ _cursor = c; }
    inline constexpr auto	gapi_version (void) const	{ return _gapi; }
    inline constexpr auto	msaa (void) const		{ return _msaa; }
    inline constexpr bool	flag (Flag f) const		{ return get_bit (_flags, uint8_t(f)); }
    inline constexpr void	set_flag (Flag f, bool v =true)	{ return set_bit (_flags, uint8_t(f), v); }
private:
    Rect	_area;
    extid_t	_parent;
    Type	_type;
    State	_state;
    Cursor	_cursor;
    uint8_t	_flags;
    uint8_t	_gapi;
    MSAA	_msaa;
};
#define SIGNATURE_ui_WindowInfo	"(" SIGNATURE_ui_Rect "qyyyyyy)"

//}}}-------------------------------------------------------------------
//{{{ PScreen

// A screen manages windows and renders their contents
class PScreen : public Proxy {
    DECLARE_INTERFACE (Screen,
	(draw, "ay")
	(get_info, "")
	(open, SIGNATURE_ui_WindowInfo)
	(close, "")
    )
public:
    using drawlist_t	= memblock;
public:
    explicit	PScreen (mrid_t caller)		: Proxy (caller) {}
    void	get_info (void) const		{ send (m_get_info()); }
    void	close (void) const		{ send (m_close()); }
    void	open (const WindowInfo& wi) const { send (m_open(), wi); }
    drawlist_t	begin_draw (void) const		{ return drawlist_t(4); }
    void	end_draw (drawlist_t&& d) const
		    { ostream(d) << uint32_t(d.size()-4); recreate_msg (m_draw(), move(d)); }
    bool	has_outgoing_draw (void) const	{ return has_outgoing_msg (m_draw()); }
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() == m_draw())
	    o->Screen_draw (msg.read().read<cmemlink>());
	else if (msg.method() == m_get_info())
	    o->Screen_get_info();
	else if (msg.method() == m_open())
	    o->Screen_open (msg.read().read<WindowInfo>());
	else if (msg.method() == m_close())
	    o->Screen_close();
	else
	    return false;
	return true;
    }
};

//}}}----------------------------------------------------------------------
//{{{ PScreenR

class PScreenR : public ProxyR {
    DECLARE_INTERFACE (ScreenR,
	(event, SIGNATURE_ui_Event)
	(expose, "")
	(resize, SIGNATURE_ui_WindowInfo)
	(screen_info, SIGNATURE_ui_ScreenInfo)
    )
public:
    constexpr	PScreenR (const Msg::Link& l)	: ProxyR (l) {}
    void	event (const Event& e) const	{ send (m_event(), e); }
    void	expose (void) const		{ send (m_expose()); }
    void	resize (const WindowInfo& wi) const
		    { send (m_resize(), wi); }
    void	screen_info (const ScreenInfo& si) const
		    { send (m_screen_info(), si); }
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() == m_event())
	    o->ScreenR_event (msg.read().read<Event>());
	else if (msg.method() == m_expose())
	    o->ScreenR_expose();
	else if (msg.method() == m_resize())
	    o->ScreenR_resize (msg.read().read<WindowInfo>());
	else if (msg.method() == m_screen_info())
	    o->ScreenR_screen_info (msg.read().read<ScreenInfo>());
	else
	    return false;
	return true;
    }
};

//}}}----------------------------------------------------------------------

} // namespace ui
} // namespace cwiclo
