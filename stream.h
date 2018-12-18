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
    using const_pointer		= cmemlink::const_pointer;
    using pointer		= const_pointer;
    enum { is_reading = true, is_writing = false, is_sizing = false };
public:
    inline constexpr		istream (pointer p, pointer e)		: _p(p),_e(e) {}
    inline constexpr		istream (pointer p, streamsize sz)	: istream(p,p+sz) {}
    inline constexpr		istream (const cmemlink& m)		: istream(m.data(),m.size()) {}
    inline constexpr		istream (const istream& is) = default;
    inline constexpr auto	end (void) const __restrict__		{ return _e; }
    inline constexpr streamsize	remaining (void) const __restrict__	{ return end()-_p; }
    template <typename T = char>
    inline auto			ptr (void) const __restrict__		{ return reinterpret_cast<const T*>(_p); }
    inline void			skip (streamsize sz) __restrict__	{ seek (_p + sz); }
    inline void			unread (streamsize sz) __restrict__	{ seek (_p - sz); }
    inline void			align (streamsize g) __restrict__	{ seek (alignptr(g)); }
    inline streamsize		alignsz (streamsize g) const		{ return alignptr(g) - _p; }
    inline bool			can_align (streamsize g) const		{ return alignptr(g) <= _e; }
    inline bool			aligned (streamsize g) const		{ return alignptr(g) == _p; }
    inline void			read (void* __restrict__ p, streamsize sz) __restrict__ {
				    assert (remaining() >= sz);
				    copy_n (_p, sz, p); skip(sz);
				}
    const char*			read_strz (void) {
				    const char* __restrict__ v = ptr<char>();
				    auto se = static_cast<pointer>(memchr (v, 0, remaining()));
				    if (!se)
					return nullptr;
				    seek (se+1);
				    return v;
				}
    template <typename T>
    inline auto&		read_trivial (void) __restrict__ {
				    const T* __restrict__ p = ptr<T>();
				    skip(sizeof(T));
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
    inline void			seek (pointer p) __restrict__			{ assert(p <= end()); _p = p; }
    inline pointer		alignptr (streamsize g) const __restrict__	{ return pointer (Align (uintptr_t(_p), g)); }
private:
    pointer			_p;
    const pointer		_e;
};

//}}}-------------------------------------------------------------------
//{{{ ostream

class ostream {
public:
    using pointer		= cmemlink::pointer;
    using const_pointer		= cmemlink::const_pointer;
    enum { is_reading = false, is_writing = true, is_sizing = false };
public:
    inline constexpr		ostream (pointer p, const_pointer e)	: _p(p),_e(e) {}
    inline constexpr		ostream (pointer p, streamsize sz)	: ostream(p,p+sz) {}
    inline constexpr		ostream (memlink& m)			: ostream(m.data(),m.size()) {}
    inline constexpr		ostream (const ostream& os) = default;
    inline constexpr auto	end (void) const __restrict__		{ return _e; }
    inline constexpr streamsize	remaining (void) const __restrict__	{ return end()-_p; }
    template <typename T = char>
    inline auto			ptr (void) __restrict__	{ return reinterpret_cast<T*>(_p); }
    inline void			skip (streamsize sz) __restrict__ {
				    pointer __restrict__ p = _p;
				    assert (p+sz <= end());
				    for (auto i = 0u; i < sz; ++i)
					*p++ = 0;
				    _p = p;
				}
    inline void			align (streamsize g) __restrict__ {
				    pointer __restrict__ p = _p;
				    while (uintptr_t(p) % g) {
					assert (p+1 <= end());
					*p++ = 0;
				    }
				    _p = p;
				}
    inline streamsize		alignsz (streamsize g) const	{ return alignptr(g) - _p; }
    inline bool			can_align (streamsize g) const	{ return alignptr(g) <= _e; }
    inline bool			aligned (streamsize g) const	{ return alignptr(g) == _p; }
    inline void			write (const void* __restrict__ p, streamsize sz) __restrict__ {
				    assert (remaining() >= sz);
				    _p = copy_n (p, sz, _p);
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
    inline void			seek (pointer p) __restrict__
				    { assert(p < end()); _p = p; }
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
    using const_pointer		= cmemlink::const_pointer;
    enum { is_reading = false, is_writing = false, is_sizing = true };
public:
    inline constexpr		sstream (void)		: _sz() {}
    inline constexpr		sstream (const sstream& ss) = default;
    inline constexpr auto	size (void) const	{ return _sz; }
    inline constexpr streamsize	remaining (void) const	{ return UINT32_MAX; }
    template <typename T = char>
    inline constexpr T*		ptr (void)	{ return nullptr; }
    inline constexpr auto	end (void)	{ return ptr(); }
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
inline auto stream_size_of (const T& v) { sstream ss; ss << v; return ss.size(); }

template <typename... Args>
inline auto variadic_stream_size (const Args&... args)
    { sstream ss; (ss << ... << args); return ss.size(); }

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
