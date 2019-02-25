// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

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
    inline constexpr		istream (const_pointer p, const_pointer e)	: _p(p),_e(e) {}
    inline constexpr		istream (const_pointer p, streamsize sz)	: istream(p,p+sz) {}
    inline constexpr		istream (const cmemlink& m)		: istream(m.data(),m.size()) {}
    inline constexpr		istream (const istream& is) = default;
    template <typename T, streamsize N>
    inline constexpr		istream (const T (&a)[N])		: istream(a, N*sizeof(T)) { static_assert (is_trivial<T>::value, "array ctor only works for trivial types"); }
    inline constexpr auto	begin (void) const __restrict__		{ return _p; }
    inline constexpr auto	end (void) const __restrict__		{ return _e; }
    inline constexpr streamsize	size (void) const __restrict__		{ return end()-begin(); }
    inline constexpr auto	remaining (void) const __restrict__	{ return size(); }
    inline constexpr auto	iat (streampos n) const __restrict__	{ return begin()+n; }
    template <typename T = char>
    inline auto			ptr (void) const __restrict__		{ return reinterpret_cast<const T*>(begin()); }
    inline void			seek (const_pointer p) __restrict__	{ assert (p <= end()); _p = p; }
    inline void			skip (streamsize sz) __restrict__	{ seek (iat(sz)); }
    inline void			unread (streamsize sz) __restrict__	{ seek (begin() - sz); }
    inline void			align (streamsize g) __restrict__	{ seek (alignptr(g)); }
    inline streamsize		alignsz (streamsize g) const		{ return alignptr(g) - begin(); }
    inline bool			can_align (streamsize g) const		{ return alignptr(g) <= end(); }
    inline bool			aligned (streamsize g) const		{ return alignptr(g) == begin(); }
    inline void			read (void* __restrict__ p, streamsize sz) __restrict__ {
				    assert (remaining() >= sz);
				    copy_n (begin(), sz, p);
				    skip (sz);
				}
    const char*			read_strz (void) {
				    const char* __restrict__ v = ptr<char>();
				    auto se = static_cast<const_pointer>(memchr (v, 0, remaining()));
				    if (!se)
					return nullptr;
				    seek (se+1);
				    return v;
				}
    template <typename T>
    inline auto&		read_trivial (void) __restrict__ {
				    const T* __restrict__ p = ptr<T>();
				    skip (sizeof(T));
				    return *p;
				}
    template <typename T>
    inline void			read_trivial (T& v) __restrict__ { v = read_trivial<T>(); }
    template <typename T>
    inline void			read_trivial_unaligned (T& v) __restrict__ {
				    #if __x86__
					v = read_trivial (v);
				    #else
					read (&v, sizeof(v));
				    #endif
				}
    template <typename T>
    inline auto&		operator>> (T& v) {
				    if constexpr (is_trivial<T>::value)
					read_trivial (v);
				    else
					v.read (*this);
				    return *this;
				}
    template <typename T>
    inline decltype(auto)	readv (void) {
				    if constexpr (is_trivial<T>::value)
					return read_trivial<T>();
				    else { T v; v.read (*this); return v; }
				}
protected:
    inline const_pointer	alignptr (streamsize g) const __restrict__	{ return const_pointer (Align (uintptr_t(_p), g)); }
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
    inline constexpr		ostream (memlink& m)			: ostream(m.data(),m.size()) {}
    inline constexpr		ostream (const ostream& os) = default;
    template <typename T, streamsize N>
    inline constexpr		ostream (T (&a)[N])			: ostream(a, N*sizeof(T)) { static_assert (is_trivial<T>::value, "array ctor only works for trivial types"); }
    inline constexpr auto	begin (void) const __restrict__		{ return _p; }
    inline constexpr auto	end (void) const __restrict__		{ return _e; }
    inline constexpr streamsize	size (void) const __restrict__		{ return end()-begin(); }
    inline constexpr auto	remaining (void) const __restrict__	{ return size(); }
    template <typename T = char>
    inline auto			ptr (void) __restrict__			{ return reinterpret_cast<T*>(begin()); }
    inline void			seek (pointer p) __restrict__		{ assert (p <= end()); _p = p; }
    inline void			skip (streamsize sz) __restrict__	{ seek (fill_n (begin(), sz, 0)); }
    inline void			align (streamsize g) __restrict__ {
				    pointer __restrict__ p = begin();
				    while (uintptr_t(p) % g) {
					assert (p+1 <= end());
					*p++ = 0;
				    }
				    seek (p);
				}
    inline streamsize		alignsz (streamsize g) const	{ return alignptr(g) - begin(); }
    inline bool			can_align (streamsize g) const	{ return alignptr(g) <= end(); }
    inline bool			aligned (streamsize g) const	{ return alignptr(g) == begin(); }
    inline void			write (const void* __restrict__ p, streamsize sz) __restrict__ {
				    assert (remaining() >= sz);
				    seek (copy_n (p, sz, begin()));
				}
    inline void			write_strz (const char* s)	{ write (s, strlen(s)+1); }
    template <typename T>
    inline void			write_trivial (const T& v) __restrict__ {
				    assert(remaining() >= sizeof(T));
				    T* __restrict__ p = ptr<T>(); *p = v;
				    _p += sizeof(T);
				}
    template <typename T>
    inline void			write_trivial_unaligned (const T& v) __restrict__ {
				    #if __x86__
					write_trivial (v);
				    #else
					write (&v, sizeof(v));
				    #endif
				}
    template <typename T>
    inline auto&		operator<< (const T& v) {
				    if constexpr (is_trivial<T>::value)
					write_trivial (v);
				    else
					v.write (*this);
				    return *this;
				}
private:
    inline const_pointer	alignptr (streamsize g) const __restrict__
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
    inline constexpr		sstream (void)		: _sz() {}
    inline constexpr		sstream (const sstream& ss) = default;
    template <typename T = char>
    inline constexpr T*		ptr (void)	{ return nullptr; }
    inline constexpr auto	begin (void)	{ return ptr(); }
    inline constexpr auto	end (void)	{ return ptr(); }
    inline constexpr auto	size (void) const	{ return _sz; }
    inline constexpr streamsize	remaining (void) const	{ return numeric_limits<size_type>::max(); }
    inline void			skip (streamsize sz)	{ _sz += sz; }
    inline void			align (streamsize g)	{ _sz = Align (_sz, g); }
    inline constexpr streamsize	alignsz (streamsize g) const	{ return Align(_sz,g) - _sz; }
    inline constexpr bool	can_align (streamsize) const	{ return true; }
    inline constexpr bool	aligned (streamsize g) const	{ return Align(_sz,g) == _sz; }
    inline void			write (const void*, streamsize sz) { skip (sz); }
    inline void			write_strz (const char* s)	{ write (s, strlen(s)+1); }
    template <typename T>
    inline void			write_trivial (const T& v)	{ write (&v, sizeof(v)); }
    template <typename T>
    inline void			write_trivial_unaligned (const T& v)	{ write (&v, sizeof(v)); }
    template <typename T>
    inline auto&		operator<< (const T& v) {
				    if constexpr (is_trivial<T>::value)
					write_trivial (v);
				    else
					v.write (*this);
				    return *this;
				}
private:
    streampos			_sz;
};

//}}}-------------------------------------------------------------------
//{{{ stream_size_of and stream_align

/// Returns the size of the given object. Do not overload - use sstream.
template <typename T>
inline auto stream_size_of (const T& v) { return (sstream() << v).size(); }

template <typename... Args>
inline auto variadic_stream_size (const Args&... args)
    { return (sstream() << ... << args).size(); }

/// Returns the recommended stream alignment for type \p T. Override with STREAM_ALIGN.
template <typename T>
struct stream_align { static constexpr const streamsize value = alignof(T); };

template <typename T>
inline constexpr auto stream_align_of (const T&) { return stream_align<T>::value; }

#define STREAM_ALIGN(type,grain)	\
    namespace cwiclo { template <> struct stream_align<type> { static constexpr const streamsize value = grain; };}

//}}}-------------------------------------------------------------------
//{{{ Stream functors

namespace ios {

/// Stream functor to allow inline align() calls.
class align {
public:
    constexpr explicit	align (streamsize grain = c_DefaultAlignment) : _grain(grain) {}
    inline void		read (istream& is) const	{ is.align (_grain); }
    inline void		write (ostream& os) const	{ os.align (_grain); }
    inline void		write (sstream& ss) const	{ ss.align (_grain); }
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
    inline void		read (istream& is) const	{ is.skip (_n); }
    inline void		write (ostream& os) const	{ os.skip (_n); }
    inline void		write (sstream& ss) const	{ ss.skip (_n); }
private:
    const streamsize	_n;
};

inline static auto& operator>> (istream& is, const align& v) { v.read (is); return is; }
inline static auto& operator>> (istream& is, const skip& v) { v.read (is); return is; }

} // namespace ios
} // namespace cwiclo
//}}}-------------------------------------------------------------------
