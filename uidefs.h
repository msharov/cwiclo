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

struct Point	{ coord_t x,y; };
struct Size	{ dim_t	w,h; };
union Rect	{
    struct { coord_t x,y; dim_t w,h; };
    struct { Point xy; Size wh; };
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
    enum class HAlign : uint8_t { Left, Center, Right };
    enum class VAlign : uint8_t { Top, Center, Bottom };
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
    constexpr void	read (istream& is)	{ is.readt (*this); }
    constexpr void	write (ostream& os)	{ os.writet (*this); }
    constexpr void	write (sstream& ss)	{ ss.writet (*this); }
private:
    uint8_t		_level:4;
    uint8_t		_halign:2;
    uint8_t		_valign:2;
    Type		_type;
    widgetid_t		_id;
};
#define SIGNATURE_WidgetLayout	"(yyq)"

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
    using key_t	= uint32_t;
    enum class Type : uint8_t {
	None,
	KeyDown,
	Selection
    };
public:
    constexpr		Event (void)
			    :_src(wid_None),_type(Type::None),_mods(0),_key(0) {}
    constexpr		Event (Type t, key_t k, widgetid_t src = wid_None)
			    :_src(src),_type(t),_mods(0),_key(k) {}
    constexpr		Event (Type t, const Point& pt, uint8_t mods = 0, widgetid_t src = wid_None)
			    :_src(src),_type(t),_mods(mods),_pt(pt) {}
    constexpr		Event (Type t, const Size& sz, uint8_t mods = 0, widgetid_t src = wid_None)
			    :_src(src),_type(t),_mods(mods),_sz(sz) {}
    constexpr auto	src (void) const	{ return _src; }
    constexpr auto	type (void) const	{ return _type; }
    constexpr auto&	loc (void) const	{ return _pt; }
    constexpr auto	selection_start (void) const	{ return _sz.w; }
    constexpr auto	selection_end (void) const	{ return _sz.h; }
    constexpr auto	key (void) const	{ return _key; }
    constexpr void	read (istream& is)	{ is.readt (*this); }
    template <typename Stm>
    constexpr void	write (Stm& os) const	{ os.writet (*this); }
private:
    widgetid_t	_src;
    Type	_type;
    uint8_t	_mods;
    union {
	Point	_pt;
	Size	_sz;
	key_t	_key;
    };
};
#define SIGNATURE_Event	"(qyyu)"

//}}}-------------------------------------------------------------------

} // namespace ui
} // namespace cwiclo
