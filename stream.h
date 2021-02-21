// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "memblock.h"

//{{{ Stream-related types ---------------------------------------------
namespace cwiclo {

using streamsize = cmemlink::size_type;
using streampos = streamsize;

/// Returns stream properties for given type
template <typename T>
class type_stream_traits {
    // Define has_member_function templates to check for
    // classes implementing stream read and write functions.
    HAS_MEMBER_FUNCTION (stream_read, read, void (O::*)(istream&));
    HAS_MEMBER_FUNCTION (stream_readu, readu, void (O::*)(istream&));
    HAS_MEMBER_FUNCTION (create_from_stream, create_from_stream, O (*)(istream&));

    static constexpr streamsize read_alignment (void) {
	if constexpr (has_read || has_write)
	    return T::stream_alignment;
	return alignof(T);
    }
public:
    static constexpr const bool has_create_from_stream = has_member_function_create_from_stream<T>::value;
    static constexpr const bool has_read = has_member_function_stream_read<T>::value || !is_trivially_assignable<T>::value;
    static constexpr const bool has_readu = has_member_function_stream_readu<T>::value || !is_trivially_assignable<T>::value;
    static constexpr const bool has_write = has_read;
    static constexpr const bool has_writeu = has_readu;
    static constexpr const streamsize alignment = read_alignment();
};

//}}}-------------------------------------------------------------------
//{{{ istream

class istream {
public:
    using value_type		= char;
    using pointer		= value_type*;
    using const_pointer		= const value_type*;
    using reference		= value_type&;
    using const_reference	= const value_type&;
    using size_type		= streamsize;
    using difference_type	= streampos;
    using const_iterator	= const_pointer;
    using iterator		= pointer;
    enum { is_reading = true, is_writing = false, is_sizing = false };
public:
    inline constexpr		istream (const_pointer p, const_pointer e)	: _p(p),_e(e) {}
    inline constexpr		istream (const_pointer p, streamsize sz)	: istream(p,p+sz) {}
    inline constexpr		istream (const void* p, streamsize sz)	: istream(static_cast<const_pointer>(p),sz) {}
    inline constexpr		istream (const cmemlink& m)		: istream(m.data(),m.size()) {}
    inline constexpr		istream (const memblaz& m)		: istream(m.data(),m.size()) {}
    inline constexpr		istream (const istream& is) = default;
    template <typename T, streamsize N>
    inline constexpr		istream (const T (&a)[N])		: istream(a, N*sizeof(T)) { static_assert (is_trivial<T>::value, "array ctor only works for trivial types"); }
    inline constexpr auto	begin (void) const __restrict__		{ return _p; }
    inline constexpr auto	end (void) const __restrict__		{ return _e; }
    inline constexpr streamsize	size (void) const __restrict__		{ return end()-begin(); }
    inline constexpr auto	remaining (void) const __restrict__	{ return size(); }
    inline constexpr auto	iat (streampos n) const __restrict__	{ assert (n <= size()); return begin()+n; }
    template <typename T = char>
    inline constexpr auto	ptr (void) const __restrict__		{ assert (aligned (alignof(T))); return pointer_cast<T>(begin()); }
    inline constexpr void	seek (const_pointer p) __restrict__	{ assert (p <= end()); _p = p; }
    inline constexpr void	skip (streamsize sz) __restrict__	{ seek (iat(sz)); }
    inline constexpr void	unread (streamsize sz) __restrict__	{ seek (begin() - sz); }
    inline constexpr void	align (streamsize g) __restrict__	{ seek (alignptr(g)); }
    inline constexpr streamsize	alignsz (streamsize g) const __restrict__	{ return alignptr(g) - begin(); }
    inline constexpr bool	can_align (streamsize g) const __restrict__	{ return alignptr(g) <= end(); }
    inline constexpr bool	aligned (streamsize g) const __restrict__	{ return divisible_by(_p, g); }
    inline constexpr void	read (void* __restrict__ p, streamsize sz) __restrict__ {
				    assert (remaining() >= sz);
				    copy_n (begin(), sz, static_cast<pointer>(p));
				    skip (sz);
				}
    inline constexpr auto	read_strz (void) {
				    auto v = ptr();
				    auto se = static_cast<const_pointer>(__builtin_memchr (v, 0, remaining()));
				    if (!se)
					return v = nullptr;
				    seek (se+1);
				    return v;
				}
    template <typename T>
    inline constexpr auto&	readt (void) __restrict__ { auto p = ptr<T>(); skip (sizeof(T)); return *p; }
    template <typename T>
    inline constexpr void	readt (T& v) __restrict__ { v = readt<T>(); }
    template <typename T>
    inline constexpr void	read (T& v) {
				    if constexpr (!type_stream_traits<T>::has_read)
					readt (v);
				    else
					v.read (*this);
				}
    template <typename T>
    inline constexpr decltype(auto) read (void) {
				    if constexpr (type_stream_traits<T>::has_create_from_stream)
					return T::create_from_stream (*this);
				    else if constexpr (!type_stream_traits<T>::has_read)
					return readt<T>();
				    else { T v; v.read (*this); return v; }
				}
    template <typename T>
    #if __x86__			// x86 can do direct unaligned reads
    inline constexpr void	readtu (T& v) __restrict__ {
				    auto p = pointer_cast<T>(begin());
				    skip (sizeof(T));
				    v = *p;
				}
    #else
    inline constexpr void	readtu (T& v) __restrict__ { read (&v, sizeof(v)); }
    #endif
    template <typename T>
    inline constexpr auto	readu (void) {
				    T v;
				    if constexpr (!type_stream_traits<T>::has_read && !type_stream_traits<T>::has_readu)
					readtu<T>(v);
				    else if constexpr (type_stream_traits<T>::has_read)
					v.read (*this);
				    else
					v.readu (*this);
				    return v;
				}
    template <typename T>
    inline constexpr auto&	operator>> (T& v) { read(v); return *this; }
    template <typename T>
    inline constexpr auto&	operator>> (T&& v) { read(v); return *this; }
protected:
    inline constexpr const_pointer alignptr (streamsize g) const __restrict__
				    { return assume_aligned (ceilg (_p, g), g); }
private:
    const_pointer		_p;
    const const_pointer		_e;
};

//}}}-------------------------------------------------------------------
//{{{ ostream

class ostream {
public:
    using value_type		= istream::value_type;
    using pointer		= istream::pointer;
    using const_pointer		= istream::const_pointer;
    using reference		= istream::reference;
    using const_reference	= istream::const_reference;
    using size_type		= istream::size_type;
    using difference_type	= istream::difference_type;
    using const_iterator	= const_pointer;
    using iterator		= pointer;
    enum { is_reading = false, is_writing = true, is_sizing = false };
public:
    inline constexpr		ostream (pointer p, const_pointer e)	: _p(p),_e(e) {}
    inline constexpr		ostream (pointer p, streamsize sz)	: ostream(p,p+sz) {}
    inline constexpr		ostream (void* p, streamsize sz)	: ostream(static_cast<pointer>(p),sz) {}
    inline constexpr		ostream (memlink& m)			: ostream(m.data(),m.size()) {}
    inline constexpr		ostream (memblaz& m)			: ostream(m.data(),m.size()) {}
    inline constexpr		ostream (const ostream& os) = default;
    template <typename T, streamsize N>
    inline constexpr		ostream (T (&a)[N])			: ostream(a, N*sizeof(T)) { static_assert (is_trivial<T>::value, "array ctor only works for trivial types"); }
    inline constexpr auto	begin (void) __restrict__		{ return _p; }
    inline constexpr auto	begin (void) const __restrict__		{ return _p; }
    inline constexpr auto	end (void) const __restrict__		{ return _e; }
    inline constexpr streamsize	size (void) const __restrict__		{ return end()-begin(); }
    inline constexpr auto	remaining (void) const __restrict__	{ return size(); }
    template <typename T = char>
    inline constexpr auto	ptr (void) __restrict__			{ assert (aligned (alignof(T))); return pointer_cast<T>(begin()); }
    inline constexpr void	seek (pointer p) __restrict__		{ assert (p <= end()); _p = p; }
    inline constexpr void	skip (streamsize sz) __restrict__	{ seek (begin()+sz); }
    inline constexpr void	zero (streamsize sz) __restrict__	{ seek (zero_fill_n (begin(), sz)); }
    inline constexpr void	align (streamsize g) __restrict__ {
				    assert (can_align(g));
				    pointer __restrict__ p = begin();
				    while (!divisible_by(p,g))
					*p++ = 0;
				    seek (assume_aligned (p,g));
				}
    inline constexpr streamsize	alignsz (streamsize g) const __restrict__	{ return alignptr(g) - begin(); }
    inline constexpr bool	can_align (streamsize g) const __restrict__	{ return alignptr(g) <= end(); }
    inline constexpr bool	aligned (streamsize g) const __restrict__	{ return divisible_by (_p, g); }
    inline constexpr void	write (const void* __restrict__ p, streamsize sz) __restrict__ {
				    assert (remaining() >= sz);
				    seek (copy_n (static_cast<const_pointer>(p), sz, begin()));
				}
    inline constexpr void	write_strz (const char* s) __restrict__	{ write (s, zstr::length(s)+1); }
    template <typename T>
    inline constexpr void	writet (const T& v) __restrict__ {
				    assert (remaining() >= sizeof(T));
				    *ptr<T>() = v;
				    skip (sizeof(T));
				}
    template <typename T>
    inline constexpr void	write (const T& v) {
				    if constexpr (!type_stream_traits<T>::has_write)
					writet (v);
				    else
					v.write (*this);
				}
    template <typename T>
    #if __x86__			// x86 can do direct unaligned writes
    inline constexpr void	writetu (const T& v) __restrict__ {
				    assert (remaining() >= sizeof(T));
				    *pointer_cast<T>(begin()) = v;
				    skip (sizeof(T));
				}
    #else
    inline constexpr void	writetu (const T& v) __restrict__ { write (&v, sizeof(v)); }
    #endif
    template <typename T>
    inline constexpr void	writeu (const T& v) {
				    if constexpr (!type_stream_traits<T>::has_write && !type_stream_traits<T>::has_writeu)
					writetu (v);
				    else if constexpr (type_stream_traits<T>::has_write)
					v.write (*this);
				    else
					v.writeu (*this);
				}
    template <typename T>
    inline constexpr auto&	operator<< (const T& v) { write(v); return *this; }
    template <typename T>
    constexpr void		write_array (const T* a, uint32_t n);
    template <typename T, cmemlink::size_type N>
    constexpr void		write_array (const T (&a)[N])	{ write_array (ARRAY_BLOCK(a)); }
    template <typename T, cmemlink::size_type N>
    inline constexpr auto&	operator<< (const T (&a)[N])	{ write_array(a); return *this; }
    constexpr void		write_string (const char* s, uint32_t n) __restrict__
				    { writet (uint32_t(n+1)); write (s, n); do { zero(1); } while (!aligned(4)); }
    template <cmemlink::size_type N>
    inline constexpr void	write_string (const char (&s)[N]) __restrict__ { write_string (ARRAY_BLOCK(s)); }
    template <cmemlink::size_type N>
    inline constexpr auto&	operator<< (const char (&s)[N]) __restrict__ { write_string(s); return *this; }
private:
    inline constexpr const_pointer alignptr (streamsize g) const __restrict__
				    { return assume_aligned (ceilg (_p, g), g); }
private:
    pointer			_p;
    const const_pointer		_e;
};

//}}}-------------------------------------------------------------------
//{{{ sstream

class sstream {
public:
    using value_type		= istream::value_type;
    using pointer		= istream::pointer;
    using const_pointer		= istream::const_pointer;
    using reference		= istream::reference;
    using const_reference	= istream::const_reference;
    using size_type		= istream::size_type;
    using difference_type	= istream::difference_type;
    using const_iterator	= const_pointer;
    using iterator		= pointer;
    enum { is_reading = false, is_writing = false, is_sizing = true };
public:
    inline constexpr		sstream (void)		: _sz() {}
    inline constexpr		sstream (const sstream& ss) = default;
    inline constexpr iterator	begin (void)		{ return nullptr; }
    inline constexpr const_iterator begin (void) const	{ return nullptr; }
    inline constexpr const_iterator end (void) const	{ return nullptr; }
    template <typename T = char>
    inline constexpr auto	ptr (void)		{ assert (aligned (alignof(T))); return pointer_cast<T>(begin()); }
    inline constexpr auto	size (void) const	{ return _sz; }
    inline constexpr streamsize	remaining (void) const	{ return numeric_limits<size_type>::max(); }
    inline constexpr void	skip (streamsize sz)	{ _sz += sz; }
    inline constexpr void	zero (streamsize sz)	{ _sz += sz; }
    inline constexpr void	align (streamsize g)	{ _sz = ceilg (_sz, g); }
    inline constexpr streamsize	alignsz (streamsize g) const	{ return ceilg(_sz,g) - _sz; }
    inline constexpr bool	can_align (streamsize) const	{ return true; }
    inline constexpr bool	aligned (streamsize g) const	{ return divisible_by (_sz, g); }
    inline constexpr void	write (const void*, streamsize sz) { skip (sz); }
    inline constexpr void	write_strz (const char* s)	{ write (s, zstr::length(s)+1); }
    template <typename T>
    inline constexpr void	writet (const T& v)	{ write (&v, sizeof(v)); }
    template <typename T>
    inline constexpr void	writetu (const T& v)	{ write (&v, sizeof(v)); }
    template <typename T>
    inline constexpr void	write (const T& v) {
				    if constexpr (!type_stream_traits<T>::has_write)
					writet (v);
				    else
					v.write (*this);
				}
    template <typename T>
    inline constexpr void	writeu (const T& v) {
				    if constexpr (!type_stream_traits<T>::has_write && !type_stream_traits<T>::has_writeu)
					writetu (v);
				    else if constexpr (type_stream_traits<T>::has_write)
					v.write (*this);
				    else
					v.writeu (*this);
				}
    template <typename T>
    inline constexpr auto&	operator<< (const T& v) { write(v); return *this; }
    template <typename T>
    constexpr void		write_array (const T* a [[maybe_unused]], uint32_t n);
    template <typename T, cmemlink::size_type N>
    constexpr void		write_array (const T (&a)[N]) { write_array (ARRAY_BLOCK(a)); }
    template <typename T, cmemlink::size_type N>
    inline constexpr auto&	operator<< (const T (&a)[N]) { write_array(a); return *this; }
    inline constexpr void	write_string (const char* s, uint32_t n)
				    { write (uint32_t(n+1)); write (s, n); do { zero(1); } while (!aligned(4)); }
    template <cmemlink::size_type N>
    inline constexpr void	write_string (const char (&s)[N]) { write_string (ARRAY_BLOCK(s)); }
    template <cmemlink::size_type N>
    inline constexpr auto&	operator<< (const char (&s)[N]) { write_string(s); return *this; }
private:
    streampos			_sz;
};

//}}}-------------------------------------------------------------------
//{{{ stream_sizeof and stream_alignof

/// Returns the size of the given object. Do not overload - use sstream.
template <typename T>
inline constexpr auto stream_sizeof (const T& v)
    { return (sstream() << v).size(); }

template <typename... Args>
inline constexpr auto variadic_stream_sizeof (const Args&... args)
    { return (sstream() << ... << args).size(); }

template <typename T>
inline constexpr auto stream_alignof (const T&)
    { return type_stream_traits<T>::alignment; }

//}}}-------------------------------------------------------------------
//{{{ Stream functors

namespace ios {

/// Inline align()
class align {
    const streamsize	_grain;
public:
    constexpr explicit		align (streamsize grain)	: _grain(grain) {}
    inline constexpr void	read (istream& is)		{ is.align (_grain); }
    inline constexpr void	write (ostream& os) const	{ os.align (_grain); }
    inline constexpr void	write (sstream& ss) const	{ ss.align (_grain); }
};

/// Type-based alignment.
template <typename T>
class talign : public align {
public:
    constexpr explicit	talign (void) : align (type_stream_traits<T>::alignment) {}
};

/// Inline skip()
class skip {
    const streamsize	_n;
public:
    constexpr explicit 	skip (streamsize n)		: _n(n) {}
    constexpr void	read (istream& is)		{ is.skip (_n); }
    constexpr void	write (ostream& os) const	{ os.skip (_n); }
    constexpr void	write (sstream& ss) const	{ ss.skip (_n); }
};

/// Inline zero()
class zero {
    const streamsize	_n;
public:
    constexpr explicit 	zero (streamsize n)		: _n(n) {}
    constexpr void	read (istream& is)		{ is.skip (_n); }
    constexpr void	write (ostream& os) const	{ os.zero (_n); }
    constexpr void	write (sstream& ss) const	{ ss.zero (_n); }
};

// Serialization of pointers as uint64_t
template <typename T>
class ptr {
public:
    using element_type	= T;
    using pointer	= element_type*;
    using reference	= element_type&;
public:
    constexpr explicit	ptr (pointer p = nullptr)	: _p(p) {}
    constexpr auto	get (void) const		{ return _p; }
    constexpr		ptr (const ptr& p)		: _p(p.get()) {}
    constexpr auto	release (void)			{ return exchange (_p, nullptr); }
    constexpr void	reset (pointer p = nullptr)	{ _p = p; }
    constexpr		operator pointer (void) const	{ return get(); }
    constexpr explicit	operator bool (void) const	{ return get() != nullptr; }
    constexpr auto&	operator= (pointer p)		{ reset (p); return *this; }
    constexpr auto&	operator= (const ptr& p)	{ reset (p.get()); return *this; }
    constexpr auto&	operator* (void) const		{ return *get(); }
    constexpr auto	operator-> (void) const		{ return get(); }
    inline constexpr bool operator== (const ptr& p) const = default;
    inline constexpr bool operator== (pointer p) const	{ return _p == p; }
    inline constexpr auto operator<=> (const ptr& p) const = default;
    inline constexpr auto operator<=> (pointer p) const	{ return _p <=> p; }
    template <typename STM>
    constexpr void	write (STM& os) const		{ os << uint64_t(pointer_value (_p)); }
    constexpr void read (istream& is) {
	static_assert (sizeof(_p) <= sizeof(uint64_t), "ios::ptr only works for pointers <= 64bit");
	if constexpr (sizeof(_p) < sizeof(uint64_t) && endian::native == endian::big)
	    is.skip (sizeof(uint64_t)-sizeof(_p));
	is.readt (_p);
	if constexpr (sizeof(_p) < sizeof(uint64_t) && endian::native == endian::little)
	    is.skip (sizeof(uint64_t)-sizeof(_p));
    }
private:
    pointer		_p;
};

} // namespace ios
//}}}-------------------------------------------------------------------
//{{{ stream write_array

template <typename T>
constexpr void ostream::write_array (const T* __restrict__ a, uint32_t n)
{
    write (n);
    if constexpr (type_stream_traits<T>::alignment > 4)
	align (type_stream_traits<T>::alignment);
    if constexpr (!type_stream_traits<T>::has_write)
	seek (pointer_cast<value_type> (copy_n (a, n, ptr<T>())));
    else for (auto i = 0u; i < n; ++i)
	write (a[i]);
    if constexpr (type_stream_traits<T>::alignment < 4)
	align (4);
}

template <typename T>
constexpr void sstream::write_array (const T* a [[maybe_unused]], uint32_t n)
{
    write (n);
    if constexpr (type_stream_traits<T>::alignment > 4)
	align (type_stream_traits<T>::alignment);
    if constexpr (!type_stream_traits<T>::has_write)
	skip (n*sizeof(T));
    else for (auto i = 0u; i < n; ++i)
	write (a[i]);
    if constexpr (type_stream_traits<T>::alignment < 4)
	align (4);
}

} // namespace cwiclo
//}}}-------------------------------------------------------------------
