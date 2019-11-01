// This file is part of the cwiclo project
//
// Copyright (c) 2019 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "msg.h"

namespace cwiclo {
namespace ui {

class Event {
public:
    using widgetid_t	= uint16_t;
    using key_t		= uint32_t;
    using coord_t	= int16_t;
    using dim_t		= make_unsigned_t<coord_t>;
    struct Point { coord_t x,y; };
    struct Size	{ dim_t	w,h; };
    union Rect	{
	struct { coord_t x,y; dim_t w,h; };
	struct { Point xy; Size wh; };
    };
    enum class Type : uint8_t {
	None,
	KeyDown,
	Selection
    };
    enum : widgetid_t {
	wid_None,
	wid_First,
	wid_Last = numeric_limits<widgetid_t>::max()
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

} // namespace ui
} // namespace cwiclo
