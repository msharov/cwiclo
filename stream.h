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
    constexpr			istream (const_pointer p, const_pointer e)	: _p(p),_e(e) {}
    constexpr			istream (const_pointer p, streamsize sz)	: istream(p,p+sz) {}
    constexpr			istream (const void* p, streamsize sz)	: istream(static_cast<const_pointer>(p),sz) {}
    constexpr			istream (const cmemlink& m)		: istream(m.data(),m.size()) {}
    constexpr			istream (const memblaz& m)		: istream(m.data(),m.size()) {}
    constexpr			istream (const istream& is) = default;
    template <typename T, streamsize N>
    constexpr			istream (const T (&a)[N])		: istream(a, N*sizeof(T)) { static_assert (is_trivial<T>::value, "array ctor only works for trivial types"); }
    constexpr auto		begin (void) const __restrict__		{ return _p; }
    constexpr auto		end (void) const __restrict__		{ return _e; }
    constexpr streamsize	size (void) const __restrict__		{ return end()-begin(); }
    constexpr auto		remaining (void) const __restrict__	{ return size(); }
    constexpr auto		iat (streampos n) const __restrict__	{ return begin()+n; }
    template <typename T = char>
    constexpr auto		ptr (void) const __restrict__		{ return reinterpret_cast<const T*>(begin()); }
    constexpr void		seek (const_pointer p) __restrict__	{ assert (p <= end()); _p = p; }
    constexpr void		skip (streamsize sz) __restrict__	{ seek (iat(sz)); }
    constexpr void		unread (streamsize sz) __restrict__	{ seek (begin() - sz); }
    constexpr void		align (streamsize g) __restrict__	{ seek (alignptr(g)); }
    constexpr streamsize	alignsz (streamsize g) const		{ return alignptr(g) - begin(); }
    constexpr bool		can_align (streamsize g) const		{ return alignptr(g) <= end(); }
    constexpr bool		aligned (streamsize g) const		{ return alignptr(g) == begin(); }
    inline void			read (void* __restrict__ p, streamsize sz) __restrict__ {
				    assert (remaining() >= sz);
				    copy_n (begin(), sz, p);
				    skip (sz);
				}
    constexpr const char*	read_strz (void) {
				    const char* __restrict__ v = ptr<char>();
				    auto se = static_cast<const_pointer>(__builtin_memchr (v, 0, remaining()));
				    if (!se)
					return nullptr;
				    seek (se+1);
				    return v;
				}
    template <typename T>
    constexpr auto&		readt (void) __restrict__ {
				    const T* __restrict__ p = ptr<T>();
				    skip (sizeof(T));
				    return *p;
				}
    template <typename T>
    constexpr void		readt (T& v) __restrict__ { v = readt<T>(); }
    template <typename T>
    constexpr void		read (T& v) __restrict__ {
				    if constexpr (is_trivial<T>::value)
					readt (v);
				    else
					v.read (*this);
				}
    template <typename T>
    constexpr decltype(auto)	read (void) __restrict__ {
				    if constexpr (is_trivial<T>::value)
					return readt<T>();
				    else { T v; v.read (*this); return v; }
				}
    template <typename T>
    #if __x86__			// x86 can do direct unaligned reads, albeit slower
    constexpr void		readtu (T& v) __restrict__ { readt (v); }
    template <typename T> constexpr
    #else
    inline void			readtu (T& v) __restrict__ { read (&v, sizeof(v)); }
    template <typename T> inline
    #endif
    decltype(auto)		readu (void) __restrict__ {
				    T v;
				    if constexpr (is_trivial<T>::value)
					readtu<T>(v);
				    else
					v.readu (*this);
				    return v;
				}
    template <typename T>
    constexpr auto&		operator>> (T& v) { read(v); return *this; }
    template <typename T>
    constexpr auto&		operator>> (T&& v) { read(v); return *this; }
protected:
    constexpr const_pointer	alignptr (streamsize g) const __restrict__	{ return const_pointer (Align (uintptr_t(_p), g)); }
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
    constexpr			ostream (pointer p, const_pointer e)	: _p(p),_e(e) {}
    constexpr			ostream (pointer p, streamsize sz)	: ostream(p,p+sz) {}
    constexpr			ostream (void* p, streamsize sz)	: ostream(static_cast<pointer>(p),sz) {}
    constexpr			ostream (memlink& m)			: ostream(m.data(),m.size()) {}
    constexpr			ostream (memblaz& m)			: ostream(m.data(),m.size()) {}
    constexpr			ostream (const ostream& os) = default;
    template <typename T, streamsize N>
    constexpr			ostream (T (&a)[N])			: ostream(a, N*sizeof(T)) { static_assert (is_trivial<T>::value, "array ctor only works for trivial types"); }
    constexpr auto		begin (void) const __restrict__		{ return _p; }
    constexpr auto		end (void) const __restrict__		{ return _e; }
    constexpr streamsize	size (void) const __restrict__		{ return end()-begin(); }
    constexpr auto		remaining (void) const __restrict__	{ return size(); }
    template <typename T = char>
    constexpr auto		ptr (void) __restrict__			{ return reinterpret_cast<T*>(begin()); }
    constexpr void		seek (pointer p) __restrict__		{ assert (p <= end()); _p = p; }
    constexpr void		skip (streamsize sz) __restrict__	{ seek (begin()+sz); }
    inline void			zero (streamsize sz) __restrict__	{ seek (fill_n (begin(), sz, 0)); }
    constexpr void		align (streamsize g) __restrict__ {
				    assert (can_align(g));
				    pointer __restrict__ p = begin();
				    while (uintptr_t(p) % g)
					*p++ = 0;
				    seek (p);
				}
    constexpr streamsize	alignsz (streamsize g) const	{ return alignptr(g) - begin(); }
    constexpr bool		can_align (streamsize g) const	{ return alignptr(g) <= end(); }
    constexpr bool		aligned (streamsize g) const	{ return alignptr(g) == begin(); }
    inline void			write (const void* __restrict__ p, streamsize sz) __restrict__ {
				    assert (remaining() >= sz);
				    seek (copy_n (p, sz, begin()));
				}
    inline void			write_strz (const char* s)	{ write (s, strlen(s)+1); }
    template <typename T>
    constexpr void		writet (const T& v) __restrict__ {
				    assert(remaining() >= sizeof(T));
				    T* __restrict__ p = ptr<T>(); *p = v;
				    _p += sizeof(T);
				}
    template <typename T>
    constexpr void		write (const T& v) {
				    if constexpr (is_trivial<T>::value)
					writet (v);
				    else
					v.write (*this);
				}
    template <typename T>
    #if __x86__			// x86 can do direct unaligned writes, albeit slower
    constexpr void		writetu (const T& v) __restrict__ { writet (v); }
    template <typename T> constexpr
    #else
    inline void			writetu (const T& v) __restrict__ { write (&v, sizeof(v)); }
    template <typename T> inline
    #endif
    void			writeu (const T& v) __restrict__ {
				    if constexpr (is_trivial<T>::value)
					writetu (v);
				    else
					v.writeu (*this);
				}
    template <typename T>
    constexpr auto&		operator<< (const T& v) { write(v); return *this; }
private:
    constexpr const_pointer	alignptr (streamsize g) const __restrict__
				    { return const_pointer (Align (uintptr_t(_p), g)); }
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
    constexpr			sstream (void)		: _sz() {}
    constexpr			sstream (const sstream& ss) = default;
    template <typename T = char>
    constexpr T*		ptr (void)	{ return nullptr; }
    constexpr auto		begin (void)	{ return ptr(); }
    constexpr auto		end (void)	{ return ptr(); }
    constexpr auto		size (void) const	{ return _sz; }
    constexpr streamsize	remaining (void) const	{ return numeric_limits<size_type>::max(); }
    constexpr void		skip (streamsize sz)	{ _sz += sz; }
    constexpr void		zero (streamsize sz)	{ _sz += sz; }
    constexpr void		align (streamsize g)	{ _sz = Align (_sz, g); }
    constexpr streamsize	alignsz (streamsize g) const	{ return Align(_sz,g) - _sz; }
    constexpr bool		can_align (streamsize) const	{ return true; }
    constexpr bool		aligned (streamsize g) const	{ return Align(_sz,g) == _sz; }
    constexpr void		write (const void*, streamsize sz) { skip (sz); }
    constexpr void		write_strz (const char* s)	{ write (s, __builtin_strlen(s)+1); }
    template <typename T>
    constexpr void		writet (const T& v)	{ write (&v, sizeof(v)); }
    template <typename T>
    constexpr void		writetu (const T& v)	{ write (&v, sizeof(v)); }
    template <typename T>
    constexpr void		write (const T& v) {
				    if constexpr (is_trivial<T>::value)
					writet (v);
				    else
					v.write (*this);
				}
    template <typename T>
    constexpr void			writeu (const T& v) {
				    if constexpr (is_trivial<T>::value)
					writetu (v);
				    else
					v.writeu (*this);
				}
    template <typename T>
    constexpr auto&		operator<< (const T& v) { write(v); return *this; }
private:
    streampos			_sz;
};

//}}}-------------------------------------------------------------------
//{{{ stream_size_of and stream_align

/// Returns the size of the given object. Do not overload - use sstream.
template <typename T>
constexpr auto stream_size_of (const T& v) { return (sstream() << v).size(); }

template <typename... Args>
constexpr auto variadic_stream_size (const Args&... args)
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
    constexpr explicit	align (streamsize grain = c_DefaultAlignment) : _grain(grain) {}
    constexpr void	read (istream& is) const	{ is.align (_grain); }
    constexpr void	write (ostream& os) const	{ os.align (_grain); }
    constexpr void	write (sstream& ss) const	{ ss.align (_grain); }
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
    inline void		write (ostream& os) const	{ os.zero (_n); }
    constexpr void	write (sstream& ss) const	{ ss.zero (_n); }
private:
    const streamsize	_n;
};

} // namespace ios
} // namespace cwiclo
//}}}-------------------------------------------------------------------
