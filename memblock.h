// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "memory.h"

namespace cwiclo {

class istream;
class ostream;
class sstream;

class alignas(16) cmemlink {
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
public:
    constexpr		cmemlink (const_pointer p, size_type n, bool z = false)	: _data(const_cast<pointer>(p)), _size(n), _capz(z) {}
    constexpr		cmemlink (const void* p, size_type n)	: cmemlink (static_cast<const_pointer>(p), n) {}
#if __SSE2__ && NDEBUG		// turn off _sblk union member for debugging
    constexpr		cmemlink (void)				: _sblk (simd16_t::zero()) {}
    constexpr		cmemlink (const cmemlink& v)		: _sblk (v._sblk) {}
    constexpr		cmemlink (cmemlink&& v)			: _sblk (exchange (v._sblk, simd16_t::zero())) {}
    constexpr void	swap (cmemlink&& v)			{ ::cwiclo::swap (_sblk, v._sblk); }
    constexpr void	unlink (void)				{ auto zt = _zerot; _sblk = simd16_t::zero(); _capz = zt; }
#else
    constexpr		cmemlink (void)				: cmemlink (nullptr, 0, false) {}
    constexpr		cmemlink (const cmemlink& v)		: cmemlink (v._data, v._size, v._zerot) {}
    constexpr		cmemlink (cmemlink&& v)			: _data (exchange (v._data, nullptr)), _size (exchange (v._size, 0u)), _capz (exchange (v._capz, 0u)) {}
    constexpr void	swap (cmemlink&& v)			{ ::cwiclo::swap (_data, v._data); ::cwiclo::swap (_size, v._size); ::cwiclo::swap (_capz, v._capz); }
    constexpr void	unlink (void)				{ _data = nullptr; _size = 0; _capacity = 0; }
#endif
    constexpr auto&	operator= (const cmemlink& v)		{ link (v); return *this; }
    constexpr auto&	operator= (cmemlink&& v)		{ swap (move(v)); return *this; }
    constexpr auto	max_size (void) const			{ return numeric_limits<size_type>::max()/2-1; }
    constexpr auto	capacity (void) const			{ return _capacity; }
    constexpr auto	size (void) const			{ return _size; }
    [[nodiscard]] constexpr auto	empty (void) const	{ return !size(); }
    constexpr const_pointer	data (void) const		{ return _data; }
    constexpr auto	cdata (void) const			{ return data(); }
    constexpr auto	begin (void) const			{ return data(); }
    constexpr auto	cbegin (void) const			{ return begin(); }
    constexpr auto	end (void) const			{ return begin()+size(); }
    constexpr auto	cend (void) const			{ return end(); }
    constexpr auto	iat (size_type i) const			{ assert (i <= size()); return begin() + i; }
    constexpr auto	ciat (size_type i) const		{ assert (i <= size()); return cbegin() + i; }
    constexpr auto&	at (size_type i) const			{ assert (i < size()); return begin()[i]; }
    constexpr auto	p2i (const_pointer p) const		{ assert (begin() <= p && end() >= p); return begin() + (p - data()); }
    constexpr auto&	operator[] (size_type i) const		{ return at (i); }
    inline bool		operator== (const cmemlink& v) const	{ return equal (v, begin()); }
    inline bool		operator!= (const cmemlink& v) const	{ return !operator==(v); }
    constexpr void	link (pointer p, size_type n)		{ _data = p; _size = n; }
    constexpr void	link (const_pointer p, size_type n)	{ link (const_cast<pointer>(p), n); }
    constexpr void	link (pointer p, size_type n, bool z)	{ link(p,n); _zerot = z; }
    constexpr void	link (const_pointer p, size_type n, bool z)	{ link (const_cast<pointer>(p), n, z); }
    constexpr void	link (const cmemlink& v)		{ link (v.begin(), v.size(), v.zero_terminated()); }
    constexpr void	resize (size_type sz)			{ _size = sz; }
    constexpr void	clear (void)				{ resize(0); }
    void		link_read (istream& is, size_type elsize = sizeof(value_type)) noexcept;
    inline void		read (istream& is, size_type elsize = sizeof(value_type))	{ link_read (is, elsize); }
    void		write (ostream& os, size_type elsize = sizeof(value_type)) const noexcept;
    inline void		write (sstream& os, size_type elsize = sizeof(value_type)) const noexcept;
    int			write_file (const char* filename) const noexcept;
    int			write_file_atomic (const char* filename) const noexcept;
protected:
    constexpr auto&	dataw (void)				{ return _data; }
    constexpr bool	zero_terminated (void) const		{ return _zerot; }
    constexpr void	set_zero_terminated (bool b = true)	{ _zerot = b; }
    constexpr void	set_capacity (size_type c)		{ _capacity = c; }
private:
#if __SSE2__ && NDEBUG
    union {
	struct {
#endif
	    pointer		_data;			///< Pointer to the data block
	    size_type		_size alignas(8);	///< Size of the data block. Aligning _size makes ccmemlink 16 bytes on 32 and 64 bit platforms.
	    union {
		size_type	_capz;
		struct {
		    bool	_zerot:1;		///< Block is zero-terminated
		    size_type	_capacity:31;		///< Total allocated capacity. Zero when linked.
		};
	    };
#if __SSE2__ && NDEBUG
	};
	simd16_t		_sblk;			// For efficient initialization and swapping
    };
#endif
};

//----------------------------------------------------------------------

class memlink : public cmemlink {
public:
    constexpr		memlink (void)				: cmemlink() {}
    constexpr		memlink (pointer p, size_type n, bool z = false)	: cmemlink (p,n,z) {}
    constexpr		memlink (void* p, size_type n)		: memlink (static_cast<pointer>(p), n) {}
    constexpr		memlink (const memlink& v)		: cmemlink(v) {}
    constexpr		memlink (memlink&& v)			: cmemlink(move(v)) {}
			using cmemlink::data;
    constexpr pointer	data (void)				{ return dataw(); }
			using cmemlink::begin;
    constexpr iterator	begin (void)				{ return data(); }
			using cmemlink::end;
    constexpr auto	end (void)				{ return begin()+size(); }
			using cmemlink::iat;
    constexpr auto	iat (size_type i)			{ assert (i <= size()); return begin() + i; }
			using cmemlink::at;
    constexpr auto	p2i (const_pointer p)			{ assert (begin() <= p && end() >= p); return begin() + (p - data()); }
    constexpr auto&	at (size_type i)			{ assert (i < size()); return begin()[i]; }
    constexpr auto&	operator= (const memlink& v)		{ cmemlink::operator=(v); return *this; }
    constexpr auto&	operator= (memlink&& v)			{ cmemlink::operator=(move(v)); return *this; }
    constexpr auto&	operator[] (size_type i)		{ return at (i); }
    constexpr auto&	operator[] (size_type i) const		{ return at (i); }
    constexpr void	link (pointer p, size_type n)		{ cmemlink::link (p, n); }
    constexpr void	link (pointer p, size_type n, bool z)	{ cmemlink::link (p, n, z); }
    constexpr void	link (const memlink& v)			{ cmemlink::link (v); }
    constexpr void	swap (memlink&& v)			{ cmemlink::swap (move (v)); }
    iterator		insert (const_iterator start, size_type n) noexcept;
    iterator		erase (const_iterator start, size_type n) noexcept;
protected:
    constexpr		memlink (const_pointer p, size_type n, bool z = false)	: cmemlink (p,n,z) {}
    constexpr		memlink (const cmemlink& l)		: cmemlink (l) {}
};

//----------------------------------------------------------------------

class memblock : public memlink {
public:
    constexpr		memblock (void)				: memlink() {}
    inline explicit	memblock (size_type sz) noexcept	: memblock() { resize (sz); }
    inline		memblock (const_pointer p, size_type n, bool z = false)	: memlink(p,n,z) { resize(n); }
    inline		memblock (const void* p, size_type n)	: memblock(static_cast<const_pointer>(p),n) {}
    inline		memblock (const cmemlink& v)		: memlink(v) { copy_link(); }
    inline		memblock (const memblock& v)		: memlink(v) { copy_link(); }
    constexpr		memblock (memlink&& v)			: memlink(move(v)) {}
    constexpr		memblock (memblock&& v)			: memlink(move(v)) {}
    inline		~memblock (void) noexcept		{ deallocate(); }
    constexpr void	manage (pointer p, size_type n)		{ assert(!capacity() && "unlink or deallocate first"); link (p, n); set_capacity(n); }
    void		assign (const_pointer p, size_type n) noexcept;
    inline void		assign (const cmemlink& v) noexcept	{ assign (v.data(), v.size()); }
    constexpr void	assign (memblock&& v) noexcept		{ swap (move(v)); }
    inline auto&	operator= (const cmemlink& v) noexcept	{ assign (v); return *this; }
    inline auto&	operator= (const memblock& v) noexcept	{ assign (v); return *this; }
    inline auto&	operator= (memblock&& v) noexcept	{ assign (move(v)); return *this; }
    void		reserve (size_type sz) noexcept;
    void		resize (size_type sz) noexcept;
    inline void		clear (void)				{ resize(0); }
    inline void		copy_link (void) noexcept		{ resize (size()); }
    iterator		insert (const_iterator start, size_type n) noexcept;
    iterator		insert (const_iterator ip, const_pointer s, size_type n) noexcept;
    auto		insert (const_iterator ip, const cmemlink& b)
			    { return insert (ip, b.data(), b.size()); }
    void	   	append (const_pointer s, size_type n)	{ insert (end(), s, n); }
    void	   	append (const cmemlink& b)		{ insert (end(), b); }
    inline auto&	operator+= (const cmemlink& b)		{ append (b); return *this; }
    auto		operator+ (const cmemlink& b) const	{ memblock r (*this); r += b; return r; }
    iterator		erase (const_iterator start, size_type n) noexcept;
    iterator		replace (const_iterator ip, size_type ipn, const_pointer s, size_type sn) noexcept;
    auto		replace (const_iterator ip, size_type ipn, const cmemlink& b)
			    { return replace (ip, ipn, b.data(), b.size()); }
    void		shrink_to_fit (void) noexcept;
    void		deallocate (void) noexcept;
    void		read (istream& is, size_type elsize = sizeof(value_type)) noexcept;
    int			read_file (const char* filename) noexcept;
    inline static auto	create_from_file (const char* filename) noexcept
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
    using memblock::erase;
    using memblock::write;
    using memblock::write_file;
    using memblock::write_file_atomic;
public:
    constexpr		memblaz (void)				: memblock() {}
    inline explicit	memblaz (size_type n)			: memblock(n) {}
    inline		memblaz (const_pointer p, size_type n, bool z = false)	: memblock(p,n,z) {}
    inline		memblaz (const void* p, size_type n)	: memblock(p,n) {}
    inline		memblaz (const cmemlink& v)		: memblock() { assign(v); }
    inline		memblaz (const memblaz& v)		: memblock() { assign(v); }
    constexpr		memblaz (memlink&& v)			: memblock (move(v)) {}
    constexpr		memblaz (memblock&& v)			: memblock (move(v)) {}
    constexpr		memblaz (memblaz&& v)			: memblock (move(v)) {}
			~memblaz (void) noexcept;
    void		assign (const_pointer p, size_type n) noexcept;
    inline void		assign (const cmemlink& v) noexcept	{ assign (v.data(), v.size()); }
    inline void		assign (const memblaz& v) noexcept	{ assign (v.data(), v.size()); }
    constexpr void	assign (memblaz&& v) noexcept		{ swap (move(v)); }
    constexpr void	clear (void)				{ shrink(0); }
    constexpr auto&	operator[] (size_type i)		{ return at (i); }
    constexpr auto&	operator[] (size_type i) const		{ return at (i); }
    inline auto&	operator= (const cmemlink& v) noexcept	{ assign (v); return *this; }
    inline auto&	operator= (const memblaz& v) noexcept	{ assign (v); return *this; }
    inline auto&	operator= (memblaz&& v) noexcept	{ assign (move(v)); return *this; }
    inline bool		operator== (const cmemlink& v) const	{ return v == *this; }
    inline bool		operator== (const memblaz& v) const	{ return memblock::operator== (v); }
    inline bool		operator!= (const cmemlink& v) const	{ return v != *this; }
    inline bool		operator!= (const memblaz& v) const	{ return memblock::operator!= (v); }
    constexpr const memblock&	mb (void) const				{ return *this; }
    inline void		copy_link (void) noexcept		{ resize (size()); }
    void		allocate (size_type sz)	{ assert (!capacity()); memblock::resize(sz); }
    constexpr void	shrink (size_type sz)	{ assert (sz <= capacity()); memlink::resize(sz); }
    inline void		wipe (void)		{ fill_n (data(), capacity(), value_type(0)); }
    void		reserve (size_type sz) noexcept;
    void		resize (size_type sz) noexcept;
    iterator		insert (const_iterator start, size_type n) noexcept;
    iterator		insert (const_iterator ip, const_pointer s, size_type n) noexcept;
    auto		insert (const_iterator ip, const cmemlink& b)
			    { return insert (ip, b.data(), b.size()); }
    auto		insert (const_iterator ip, const memblaz& b)
			    { return insert (ip, b.data(), b.size()); }
    auto	   	append (size_type n)			{ return insert (end(), n); }
    auto	   	append (const_pointer s, size_type n)	{ return insert (end(), s, n); }
    auto	   	append (const cmemlink& b)		{ return insert (end(), b); }
    auto	   	append (const memblaz& b)		{ return insert (end(), b); }
    inline auto&	operator+= (const cmemlink& b)		{ append (b); return *this; }
    inline auto&	operator+= (const memblaz& b)		{ append (b); return *this; }
    auto		operator+ (const cmemlink& b) const	{ memblaz r (*this); r += b; return r; }
    auto		operator+ (const memblaz& b) const	{ memblaz r (*this); r += b; return r; }
    iterator		replace (const_iterator ip, size_type ipn, const_pointer s, size_type sn) noexcept;
    auto		replace (const_iterator ip, size_type ipn, const cmemlink& b)
			    { return replace (ip, ipn, b.data(), b.size()); }
    auto		replace (const_iterator ip, size_type ipn, const memblaz& b)
			    { return replace (ip, ipn, b.data(), b.size()); }
    void		shrink_to_fit (void) noexcept;
    void		deallocate (void) noexcept;
    void		read (istream& is, size_type elsize = sizeof(value_type)) noexcept;
    int			read_file (const char* filename) noexcept;
    inline static auto	create_from_file (const char* filename) noexcept
			    { memblaz r; r.read_file (filename); return r; }
};

//----------------------------------------------------------------------

/// Use with memlink-derived classes to link to a static array
#define static_link(v)	link (ArrayBlock(v))
/// Use with memlink-derived classes to allocate and link to stack space.
#define alloca_link(n)	link (alloca(n), (n))

} // namespace cwiclo
