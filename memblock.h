// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "algo.h"

namespace cwiclo {

class istream;
class ostream;
class sstream;

class cmemlink {
public:
    using value_type		= char;
    using pointer		= value_type*;
    using const_pointer		= const value_type*;
    using reference		= value_type&;
    using const_reference	= const value_type&;
    using size_type		= uint32_t;
    using difference_type	= ptrdiff_t;
    using const_iterator	= const_pointer;
    using iterator		= pointer;
    struct alignas(16) block_type {
	pointer		data;		///< Pointer to the data block
	size_type	size alignas(8);///< Used size
	bool		zerot:1;	///< True if zero-terminated
	size_type	capacity:31;	///< Allocated size
    };
public:
    inline constexpr		cmemlink (const_pointer p, size_type n, bool z = false)	: _d{const_cast<pointer>(p), n, z, 0} {}
    inline constexpr		cmemlink (const void* p, size_type n)			: cmemlink (static_cast<const_pointer>(p),n) {}
    inline constexpr		cmemlink (void)			: _d{}{}
    inline constexpr		cmemlink (const cmemlink& v)	: _d{v._d}{}
    inline constexpr		cmemlink (cmemlink&& v)		: _d (exchange (v._d, block_type{})) {}
    inline constexpr void	swap (cmemlink&& v)		{ ::cwiclo::swap (_d, v._d); }
    inline constexpr void	unlink (void)			{ auto zt = zero_terminated(); _d = block_type{}; _d.zerot = zt; }
    inline constexpr auto&	operator= (const cmemlink& v)	{ link (v); return *this; }
    inline constexpr auto&	operator= (cmemlink&& v)	{ swap (move(v)); return *this; }
    inline constexpr auto	max_size (void) const		{ return numeric_limits<size_type>::max()/2-1; }
    inline constexpr auto	capacity (void) const		{ return _d.capacity; }
    inline constexpr auto	size (void) const		{ return _d.size; }
    [[nodiscard]] inline constexpr auto	empty (void) const	{ return !size(); }
    inline constexpr const_pointer	data (void) const	{ return _d.data; }
    inline constexpr auto	cdata (void) const		{ return data(); }
    inline constexpr auto	begin (void) const		{ return data(); }
    inline constexpr auto	cbegin (void) const		{ return begin(); }
    inline constexpr auto	end (void) const		{ return begin()+size(); }
    inline constexpr auto	cend (void) const		{ return end(); }
    inline constexpr auto	iat (size_type i) const		{ assert (i <= size()); return begin() + i; }
    inline constexpr auto	ciat (size_type i) const	{ assert (i <= size()); return cbegin() + i; }
    inline constexpr auto&	at (size_type i) const		{ assert (i < size()); return begin()[i]; }
    inline constexpr auto	p2i (const_pointer p) const	{ assert (begin() <= p && end() >= p); return begin() + (p - data()); }
    inline constexpr auto&	operator[] (size_type i) const		{ return at (i); }
    inline bool			operator== (const cmemlink& v) const	{ return equal (v, begin()); }
    inline bool			operator!= (const cmemlink& v) const	{ return !operator==(v); }
    inline constexpr void	link (pointer p, size_type n)		{ _d.data = p; _d.size = n; }
    inline constexpr void	link (const_pointer p, size_type n)	{ link (const_cast<pointer>(p), n); }
    inline constexpr void	link (void* p, size_type n)		{ link (static_cast<pointer>(p), n); }
    inline constexpr void	link (const void* p, size_type n)	{ link (static_cast<const_pointer>(p), n); }
    inline constexpr void	link (const cmemlink& v)	{ link (v.begin(), v.size()); }
    inline constexpr void	resize (size_type sz)		{ _d.size = sz; }
    inline constexpr void	shrink (size_type sz)		{ assert (sz <= max(capacity(),size())); resize (sz); }
    inline constexpr void	clear (void)			{ shrink (0); }
    void		link_read (istream& is, size_type elsize = sizeof(value_type));
    inline void		read (istream& is)			{ link_read (is, sizeof(value_type)); }
    template <typename Stm>
    constexpr void	write (Stm& os) const;
    inline static constexpr cmemlink create_from_stream (istream& is);
    int			write_file (const char* filename) const;
    int			write_file_atomic (const char* filename) const;
protected:
    constexpr auto&	dataw (void)				{ return _d.data; }
    constexpr bool	zero_terminated (void) const		{ return _d.zerot; }
    constexpr void	set_zero_terminated (bool b = true)	{ _d.zerot = b; }
    constexpr void	set_capacity (size_type c)		{ _d.capacity = c; }
private:
    block_type		_d;
};

//----------------------------------------------------------------------

class memlink : public cmemlink {
public:
    inline constexpr		memlink (void)			: cmemlink() {}
    inline constexpr		memlink (pointer p, size_type n, bool z = false) : cmemlink (p,n,z) {}
    inline constexpr		memlink (void* p, size_type n)	: memlink (static_cast<pointer>(p), n) {}
    inline constexpr		memlink (const memlink& v)	: cmemlink(v) {}
    inline constexpr		memlink (memlink&& v)		: cmemlink(move(v)) {}
				using cmemlink::data;
    inline constexpr pointer	data (void)			{ return dataw(); }
				using cmemlink::begin;
    inline constexpr iterator	begin (void)			{ return data(); }
				using cmemlink::end;
    inline constexpr auto	end (void)			{ return begin()+size(); }
				using cmemlink::iat;
    inline constexpr auto	iat (size_type i)		{ assert (i <= size()); return begin() + i; }
				using cmemlink::at;
    inline constexpr auto	p2i (const_pointer p)		{ assert (begin() <= p && end() >= p); return begin() + (p - data()); }
    inline constexpr auto&	at (size_type i)		{ assert (i < size()); return begin()[i]; }
    inline constexpr auto&	operator= (const memlink& v)	{ cmemlink::operator=(v); return *this; }
    inline constexpr auto&	operator= (memlink&& v)		{ cmemlink::operator=(move(v)); return *this; }
    inline constexpr auto&	operator[] (size_type i)	{ return at (i); }
    inline constexpr auto&	operator[] (size_type i) const	{ return at (i); }
    inline constexpr void	link (pointer p, size_type n)	{ cmemlink::link (p, n); }
    inline constexpr void	link (void* p, size_type n)	{ link (static_cast<pointer>(p), n); }
    inline constexpr void	link (const memlink& v)		{ cmemlink::link (v); }
    inline constexpr void	swap (memlink&& v)		{ cmemlink::swap (move (v)); }
    iterator			insert (const_iterator start, size_type n);
    iterator			erase (const_iterator start, size_type n);
protected:
    inline constexpr		memlink (const_pointer p, size_type n, bool z = false)	: cmemlink (p,n,z) {}
    inline constexpr		memlink (const cmemlink& l)	: cmemlink (l) {}
};

//----------------------------------------------------------------------

class memblock : public memlink {
public:
    inline constexpr		memblock (void)			: memlink() {}
    inline explicit		memblock (size_type sz)		: memblock() { resize (sz); }
    inline			memblock (const_pointer p, size_type n, bool z = false)	: memlink(p,n,z) { resize(n); }
    inline			memblock (const void* p, size_type n)	: memblock(static_cast<const_pointer>(p),n) {}
    inline			memblock (const cmemlink& v)	: memlink(v) { copy_link(); }
    inline			memblock (const memblock& v)	: memlink(v) { copy_link(); }
    inline constexpr		memblock (memlink&& v)		: memlink(move(v)) {}
    inline constexpr		memblock (memblock&& v)		: memlink(move(v)) {}
    inline			~memblock (void)		{ deallocate(); }
    inline constexpr void	manage (pointer p, size_type n, size_type c)	{ link (p, n); set_capacity(c); }
    inline constexpr void	manage (pointer p, size_type n)	{ manage (p, n, n); }
    inline constexpr void	manage (void* p, size_type n, size_type c)	{ manage (static_cast<pointer>(p), n, c); }
    inline constexpr void	manage (void* p, size_type n)	{ manage (p, n, n); }
    void			assign (const_pointer p, size_type n);
    inline void			assign (const cmemlink& v)	{ assign (v.data(), v.size()); }
    inline void			assign (memlink&& v)		{ deallocate(); swap (move(v)); }
    inline constexpr void	assign (memblock&& v)		{ swap (move(v)); }
    inline auto&		operator= (const cmemlink& v)	{ assign (v); return *this; }
    inline auto&		operator= (const memblock& v)	{ assign (v); return *this; }
    inline auto&		operator= (memlink&& v)		{ assign (move(v)); return *this; }
    inline auto&		operator= (memblock&& v)	{ assign (move(v)); return *this; }
    void			reserve (size_type sz);
    void			resize (size_type sz);
    inline void			copy_link (void)		{ resize (size()); }
    iterator			insert (const_iterator start, size_type n);
    iterator			insert (const_iterator ip, const_pointer s, size_type n);
    auto			insert (const_iterator ip, const cmemlink& b)
				    { return insert (ip, b.data(), b.size()); }
    void	   		append (const_pointer s, size_type n)	{ insert (end(), s, n); }
    void	   		append (const cmemlink& b)		{ insert (end(), b); }
    inline auto&		operator+= (const cmemlink& b)		{ append (b); return *this; }
    auto			operator+ (const cmemlink& b) const	{ memblock r (*this); r += b; return r; }
    iterator			erase (const_iterator start, size_type n);
    iterator			replace (const_iterator ip, size_type ipn, const_pointer s, size_type sn);
    auto			replace (const_iterator ip, size_type ipn, const cmemlink& b)
				    { return replace (ip, ipn, b.data(), b.size()); }
    void			shrink_to_fit (void);
    void			deallocate (void);
    void			read (istream& is);
    int				read_file (const char* filename);
    inline static auto		create_from_file (const char* filename)
				    { memblock r; r.read_file (filename); return r; }
};

//----------------------------------------------------------------------

// A memblock that ensures its storage is zeroed when freed.
class memblaz : protected memblock {
public:	// memblock is a protected base to disallow calling its reserve
    using memblock::value_type;
    using memblock::pointer;
    using memblock::const_pointer;
    using memblock::reference;
    using memblock::const_reference;
    using memblock::size_type;
    using memblock::difference_type;
    using memblock::const_iterator;
    using memblock::iterator;
public:
    using memblock::size;
    using memblock::capacity;
    using memblock::data;
    using memblock::begin;
    using memblock::end;
    using memblock::iat;
    using memblock::p2i;
    using memblock::max_size;
    using memblock::empty;
    using memblock::at;
    using memblock::cdata;
    using memblock::cbegin;
    using memblock::cend;
    using memblock::link;
    using memblock::unlink;
    using memblock::manage;
    using memblock::shrink;
    using memblock::clear;
    using memblock::erase;
    using memblock::write;
    using memblock::write_file;
    using memblock::write_file_atomic;
public:
    inline constexpr		memblaz (void)			: memblock() {}
    inline explicit		memblaz (size_type n)		: memblock(n) {}
    inline			memblaz (const_pointer p, size_type n, bool z = false) : memblock(p,n,z) {}
    inline			memblaz (const void* p, size_type n) : memblock(p,n) {}
    inline			memblaz (const cmemlink& v)	: memblock() { assign(v); }
    inline			memblaz (const memblaz& v)	: memblock() { assign(v); }
    inline constexpr		memblaz (memlink&& v)		: memblock (move(v)) {}
    inline constexpr		memblaz (memblock&& v)		: memblock (move(v)) {}
    inline constexpr		memblaz (memblaz&& v)		: memblock (move(v)) {}
				~memblaz (void);
    void			assign (const_pointer p, size_type n);
    inline void			assign (const cmemlink& v)	{ assign (v.data(), v.size()); }
    inline void			assign (const memblaz& v)	{ assign (v.data(), v.size()); }
    inline constexpr void	assign (memblaz&& v)		{ swap (move(v)); }
    inline constexpr void	assign (memblock&& v)		{ wipe(); swap (move(v)); }
    inline void			assign (memlink&& v)		{ deallocate(); swap (move(v)); }
    inline constexpr void	wipe (void)			{ zero_fill_n (data(), capacity()); }
    constexpr auto&		operator[] (size_type i)	{ return at (i); }
    constexpr auto&		operator[] (size_type i) const	{ return at (i); }
    inline auto&		operator= (const cmemlink& v)	{ assign (v); return *this; }
    inline auto&		operator= (const memblaz& v)	{ assign (v); return *this; }
    inline constexpr auto&	operator= (memblaz&& v)		{ assign (move(v)); return *this; }
    inline constexpr auto&	operator= (memblock&& v)	{ assign (move(v)); return *this; }
    inline auto&		operator= (memlink&& v)		{ assign (move(v)); return *this; }
    inline bool			operator== (const cmemlink& v) const	{ return v == *this; }
    inline bool			operator== (const memblaz& v) const	{ return memblock::operator== (v); }
    inline bool			operator!= (const cmemlink& v) const	{ return v != *this; }
    inline bool			operator!= (const memblaz& v) const	{ return memblock::operator!= (v); }
    inline constexpr const memblock&	mb (void) const			{ return *this; }
    inline void			copy_link (void)		{ resize (size()); }
    void			allocate (size_type sz)		{ assert (!capacity()); memblock::resize(sz); }
    void			reserve (size_type sz);
    void			resize (size_type sz);
    iterator			insert (const_iterator start, size_type n);
    iterator			insert (const_iterator ip, const_pointer s, size_type n);
    auto			insert (const_iterator ip, const cmemlink& b)
				    { return insert (ip, b.data(), b.size()); }
    auto			insert (const_iterator ip, const memblaz& b)
				    { return insert (ip, b.data(), b.size()); }
    auto	   		append (size_type n)			{ return insert (end(), n); }
    auto	   		append (const_pointer s, size_type n)	{ return insert (end(), s, n); }
    auto	   		append (const cmemlink& b)		{ return insert (end(), b); }
    auto	   		append (const memblaz& b)		{ return insert (end(), b); }
    inline auto&		operator+= (const cmemlink& b)		{ append (b); return *this; }
    inline auto&		operator+= (const memblaz& b)		{ append (b); return *this; }
    auto			operator+ (const cmemlink& b) const	{ memblaz r (*this); r += b; return r; }
    auto			operator+ (const memblaz& b) const	{ memblaz r (*this); r += b; return r; }
    iterator			replace (const_iterator ip, size_type ipn, const_pointer s, size_type sn);
    auto			replace (const_iterator ip, size_type ipn, const cmemlink& b)
				    { return replace (ip, ipn, b.data(), b.size()); }
    auto			replace (const_iterator ip, size_type ipn, const memblaz& b)
				    { return replace (ip, ipn, b.data(), b.size()); }
    void			shrink_to_fit (void);
    void			deallocate (void);
    inline void			read (istream& is) { wipe(); memblock::read (is); }
    int				read_file (const char* filename);
    inline static auto		create_from_file (const char* filename)
				    { memblaz r; r.read_file (filename); return r; }
};

//----------------------------------------------------------------------

/// Use with memlink-derived classes to link to a static array
#define static_link(v)	link (ARRAY_BLOCK(v))
/// Use with memlink-derived classes to allocate and link to stack space.
#define alloca_link(n)	link (alloca(n), (n))

} // namespace cwiclo
