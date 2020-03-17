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

// Define has_member_function templates to check for classes
// implementing stream read and write functions.
HAS_MEMBER_FUNCTION (stream_read, read, void (O::*)(istream&));
HAS_MEMBER_FUNCTION (stream_readu, readu, void (O::*)(istream&));
HAS_MEMBER_FUNCTION (create_from_stream, create_from_stream, O (*)(istream&));

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
    inline constexpr auto	iat (streampos n) const __restrict__	{ return begin()+n; }
    template <typename T = char>
    inline constexpr auto	ptr (void) const __restrict__		{ return pointer_cast<T>(begin()); }
    inline constexpr void	seek (const_pointer p) __restrict__	{ assert (p <= end()); _p = p; }
    inline constexpr void	skip (streamsize sz) __restrict__	{ seek (iat(sz)); }
    inline constexpr void	unread (streamsize sz) __restrict__	{ seek (begin() - sz); }
    inline constexpr void	align (streamsize g) __restrict__	{ seek (alignptr(g)); }
    inline constexpr streamsize	alignsz (streamsize g) const		{ return alignptr(g) - begin(); }
    inline constexpr bool	can_align (streamsize g) const		{ return alignptr(g) <= end(); }
    inline constexpr bool	aligned (streamsize g) const		{ return divisible_by(_p, g); }
    inline void			read (void* __restrict__ p, streamsize sz) __restrict__ {
				    assert (remaining() >= sz);
				    copy_n (begin(), sz, p);
				    skip (sz);
				}
    inline constexpr auto	read_strz (void) {
				    const char* __restrict__ v = ptr<char>();
				    auto se = static_cast<const_pointer>(__builtin_memchr (v, 0, remaining()));
				    if (!se)
					return v = nullptr;
				    seek (se+1);
				    return v;
				}
    template <typename T>
    inline constexpr auto&	readt (void) __restrict__ {
				    const T* __restrict__ p = ptr<T>();
				    skip (sizeof(T));
				    return *p;
				}
    template <typename T>
    inline constexpr void	readt (T& v) __restrict__ { v = readt<T>(); }
    template <typename T>
    inline constexpr void	read (T& v) {
				    if constexpr (is_trivially_assignable<T>::value && !has_member_function_stream_read<T>::value)
					readt (v);
				    else
					v.read (*this);
				}
    template <typename T>
    inline constexpr decltype(auto) read (void) {
				    if constexpr (has_member_function_create_from_stream<T>::value)
					return T::create_from_stream (*this);
				    else if constexpr (is_trivially_assignable<T>::value && !has_member_function_stream_read<T>::value)
					return readt<T>();
				    else { T v; v.read (*this); return v; }
				}
    template <typename T>
    #if __x86__			// x86 can do direct unaligned reads, albeit slower
    inline void			readtu (T& v) __restrict__ { readt (v); }
    #else
    inline void			readtu (T& v) __restrict__ { read (&v, sizeof(v)); }
    #endif
    template <typename T>
    inline decltype(auto)	readu (void) {
				    T v;
				    if constexpr (is_trivially_assignable<T>::value && !has_member_function_stream_read<T>::value && !has_member_function_stream_readu<T>::value)
					readtu<T>(v);
				    else if constexpr (has_member_function_stream_read<T>::value && !has_member_function_stream_readu<T>::value)
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
    inline constexpr auto	begin (void) const __restrict__		{ return _p; }
    inline constexpr auto	end (void) const __restrict__		{ return _e; }
    inline constexpr streamsize	size (void) const __restrict__		{ return end()-begin(); }
    inline constexpr auto	remaining (void) const __restrict__	{ return size(); }
    template <typename T = char>
    inline constexpr auto	ptr (void) __restrict__			{ return pointer_cast<T>(begin()); }
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
    inline void			write (const void* __restrict__ p, streamsize sz) __restrict__ {
				    assert (remaining() >= sz);
				    seek (copy_n (p, sz, begin()));
				}
    inline void			write_strz (const char* s) __restrict__	{ write (s, __builtin_strlen(s)+1); }
    template <typename T>
    inline constexpr void	writet (const T& v) __restrict__ {
				    assert (remaining() >= sizeof(T));
				    T* __restrict__ p = ptr<T>(); *p = v;
				    _p += sizeof(T);
				}
    template <typename T>
    inline constexpr void	write (const T& v) {
				    if constexpr (is_trivially_copyable<T>::value && !has_member_function_stream_read<T>::value)
					writet (v);
				    else
					v.write (*this);
				}
    template <typename T>
    #if __x86__			// x86 can do direct unaligned writes, albeit slower
    inline void			writetu (const T& v) __restrict__ { writet (v); }
    #else
    inline void			writetu (const T& v) __restrict__ { write (&v, sizeof(v)); }
    #endif
    template <typename T>
    inline void			writeu (const T& v) {
				    if constexpr (is_trivially_copyable<T>::value && !has_member_function_stream_readu<T>::value && !has_member_function_stream_read<T>::value)
					writetu (v);
				    else if constexpr (has_member_function_stream_read<T>::value && !has_member_function_stream_readu<T>::value)
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
    void			write_string (const char* s, uint32_t n) __restrict__
				    { writet (uint32_t(n+1)); write (s, n); do { zero(1); } while (!aligned(4)); }
    template <cmemlink::size_type N>
    inline void			write_string (const char (&s)[N]) __restrict__ { write_string (ARRAY_BLOCK(s)); }
    template <cmemlink::size_type N>
    inline auto&		operator<< (const char (&s)[N]) __restrict__ { write_string(s); return *this; }
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
    template <typename T = char>
    inline constexpr T*		ptr (void)	{ return nullptr; }
    inline constexpr auto	begin (void)	{ return ptr(); }
    inline constexpr auto	end (void)	{ return ptr(); }
    inline constexpr auto	size (void) const	{ return _sz; }
    inline constexpr streamsize	remaining (void) const	{ return numeric_limits<size_type>::max(); }
    inline constexpr void	skip (streamsize sz)	{ _sz += sz; }
    inline constexpr void	zero (streamsize sz)	{ _sz += sz; }
    inline constexpr void	align (streamsize g)	{ _sz = ceilg (_sz, g); }
    inline constexpr streamsize	alignsz (streamsize g) const	{ return ceilg(_sz,g) - _sz; }
    inline constexpr bool	can_align (streamsize) const	{ return true; }
    inline constexpr bool	aligned (streamsize g) const	{ return divisible_by (_sz, g); }
    inline constexpr void	write (const void*, streamsize sz) { skip (sz); }
    inline constexpr void	write_strz (const char* s)	{ write (s, __builtin_strlen(s)+1); }
    template <typename T>
    inline constexpr void	writet (const T& v)	{ write (&v, sizeof(v)); }
    template <typename T>
    inline constexpr void	writetu (const T& v)	{ write (&v, sizeof(v)); }
    template <typename T>
    inline constexpr void	write (const T& v) {
				    if constexpr (is_trivially_copyable<T>::value && !has_member_function_stream_read<T>::value)
					writet (v);
				    else
					v.write (*this);
				}
    template <typename T>
    inline constexpr void	writeu (const T& v) {
				    if constexpr (is_trivially_copyable<T>::value && !has_member_function_stream_readu<T>::value && !has_member_function_stream_read<T>::value)
					writetu (v);
				    else if constexpr (has_member_function_stream_read<T>::value && !has_member_function_stream_readu<T>::value)
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
//{{{ stream_size_of and stream_align

/// Returns the size of the given object. Do not overload - use sstream.
template <typename T>
inline constexpr auto stream_size_of (const T& v)
    { return (sstream() << v).size(); }

template <typename... Args>
inline constexpr auto variadic_stream_size (const Args&... args)
    { return (sstream() << ... << args).size(); }

/// Returns the recommended stream alignment for type \p T. Override with STREAM_ALIGN.
template <typename T>
struct stream_align { static constexpr const streamsize value = alignof(T); };

template <typename T>
constexpr auto stream_align_of (const T&) { return stream_align<T>::value; }

#define STREAM_ALIGN(type,grain)	\
    namespace cwiclo { template <> struct stream_align<type> { static constexpr const streamsize value = grain; };}

//}}}-------------------------------------------------------------------
//{{{ Stream functors

namespace ios {

/// Stream functor to allow inline align() calls.
class align {
public:
    constexpr explicit		align (streamsize grain)	: _grain(grain) {}
    inline constexpr void	read (istream& is)		{ is.align (_grain); }
    inline constexpr void	write (ostream& os) const	{ os.align (_grain); }
    inline constexpr void	write (sstream& ss) const	{ ss.align (_grain); }
private:
    const streamsize	_grain;
};

/// Stream functor to allow type-based alignment.
template <typename T>
class talign : public align {
public:
    constexpr explicit	talign (void) : align (stream_align<T>::value) {}
};

/// Stream functor to allow inline skip() calls.
class skip {
public:
    constexpr explicit 	skip (streamsize n)		: _n(n) {}
    constexpr void	read (istream& is) const	{ is.skip (_n); }
    constexpr void	write (ostream& os) const	{ os.skip (_n); }
    constexpr void	write (sstream& ss) const	{ ss.skip (_n); }
private:
    const streamsize	_n;
};

/// Stream functor to allow inline zero() calls.
class zero {
public:
    constexpr explicit 	zero (streamsize n)		: _n(n) {}
    constexpr void	read (istream& is) const	{ is.skip (_n); }
    constexpr void	write (ostream& os) const	{ os.zero (_n); }
    constexpr void	write (sstream& ss) const	{ ss.zero (_n); }
private:
    const streamsize	_n;
};

} // namespace ios
//}}}-------------------------------------------------------------------
//{{{ stream write_array

template <typename T>
constexpr void ostream::write_array (const T* __restrict__ a, uint32_t n)
{
    write (n);
    if constexpr (stream_align<T>::value > 4)
	align (stream_align<T>::value);
    if constexpr (is_trivially_copyable<T>::value)
	seek (pointer_cast<value_type> (copy_n (a, n, ptr<T>())));
    else for (auto i = 0u; i < n; ++i)
	write (a[i]);
    if constexpr (stream_align<T>::value < 4)
	align (4);
}

template <typename T>
constexpr void sstream::write_array (const T* a [[maybe_unused]], uint32_t n)
{
    write (n);
    if constexpr (stream_align<T>::value > 4)
	align (stream_align<T>::value);
    if constexpr (is_trivially_copyable<T>::value)
	skip (n*sizeof(T));
    else for (auto i = 0u; i < n; ++i)
	write (a[i]);
    if constexpr (stream_align<T>::value < 4)
	align (4);
}

} // namespace cwiclo
//}}}-------------------------------------------------------------------
