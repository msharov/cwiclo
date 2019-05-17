// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "memory.h"

//{{{ Searching algorithms
namespace cwiclo {

template <typename I, typename T>
constexpr auto find (I f, I l, const T& v)
{
    while (f < l && !(*f == v))
	advance(f);
    return f;
}

template <typename I, typename P>
constexpr auto find_if (I f, I l, P p)
{
    while (f < l && !p(*f))
	advance(f);
    return f;
}

template <typename I, typename T>
constexpr I linear_search (I f, I l, const T& v)
{
    for (; f < l; advance(f))
	if (*f == v)
	    return f;
    return nullptr;
}

template <typename I, typename P>
constexpr I linear_search_if (I f, I l, P p)
{
    for (; f < l; advance(f))
	if (p(*f))
	    return f;
    return nullptr;
}

template <typename I, typename T>
constexpr auto lower_bound (I f, I l, const T& v)
{
    while (f < l) {
	auto m = next (f, uintptr_t(distance(f,l))/2);
	if (*m < v)
	    f = next(m);
	else
	    l = m;
    }
    return f;
}

template <typename I, typename T>
constexpr auto binary_search (I f, I l, const T& v)
{
    auto b = lower_bound (f, l, v);
    return (b < l && *b == v) ? b : nullptr;
}

template <typename I, typename T>
constexpr auto upper_bound (I f, I l, const T& v)
{
    while (f < l) {
	auto m = next (f, uintptr_t(distance(f,l))/2);
	if (v < *m)
	    l = m;
	else
	    f = next(m);
    }
    return l;
}

template <typename T>
constexpr int c_compare (const void* p1, const void* p2)
{
    auto& v1 = *static_cast<const T*>(p1);
    auto& v2 = *static_cast<const T*>(p2);
    return v1 < v2 ? -1 : (v2 < v1 ? 1 : 0);
}

template <typename I>
constexpr void sort (I f, I l)
{
    using value_type = typename iterator_traits<I>::value_type;
    qsort (f, distance(f,l), sizeof(value_type), c_compare<value_type>);
}

template <typename C, typename T>
inline constexpr auto find (C& c, const T& v)
    { return find (begin(c), end(c), v); }
template <typename C, typename P>
inline constexpr auto find_if (C& c, P p)
    { return find_if (begin(c), end(c), move(p)); }
template <typename C, typename T>
inline constexpr auto linear_search (C& c, const T& v)
    { return linear_search (begin(c), end(c), v); }
template <typename C, typename P>
inline constexpr auto linear_search_if (C& c, P p)
    { return linear_search_if (begin(c), end(c), move(p)); }
template <typename C, typename T>
inline constexpr auto lower_bound (C& c, const T& v)
    { return lower_bound (begin(c), end(c), v); }
template <typename C, typename T>
inline constexpr auto upper_bound (C& c, const T& v)
    { return upper_bound (begin(c), end(c), v); }
template <typename C, typename T>
inline constexpr auto binary_search (C& c, const T& v)
    { return binary_search (begin(c), end(c), v); }
template <typename C>
inline constexpr void sort (C& c)
    { sort (begin(c), end(c)); }

template <typename I1, typename I2>
bool equal_n (I1 i1, size_t n, I2 i2)
{
    using value1_type = make_unsigned_t<remove_const_t<typename iterator_traits<I1>::value_type>>;
    using value2_type = make_unsigned_t<remove_const_t<typename iterator_traits<I2>::value_type>>;
    if constexpr (is_trivial<value1_type>::value && is_same<value1_type,value2_type>::value) {
	#if __x86__ && __clang__
	    bool r;
	    __asm__("repz\tcmpsb\n\tsete\t%3":"+D"(i1),"+S"(i2),"+c"(n),"=r"(r)::"cc","memory");
	    return r;
	#elif __x86__
	    bool e,l;
	    __asm__("repz\tcmpsb":"+D"(i1),"+S"(i2),"+c"(n),"=@cce"(e),"=@ccl"(l)::"memory");
	    return e;
	#else
	    return 0 == __builtin_memcmp (i1, i2, n);
	#endif
    }
    while (n--)
	if (!(i1 == i2))
	    return false;
    return true;
}

template <typename I1, typename I2>
inline bool equal_n (I1 i1, size_t n1, I2 i2, size_t n2)
    { return n1 == n2 && equal_n (i1, n1, i2); }

template <typename I1, typename I2>
inline bool equal (I1 i1f, I1 i1l, I2 i2)
    { return equal_n (i1f, i1l-i1f, i2); }
template <typename I1, typename I2>
inline bool equal (I1 i1f, I1 i1l, I2 i2f, I2 i2l)
    { return equal_n (i1f, i1l-i1f, i2f, i2l-i2f); }

template <typename C, typename I>
inline bool equal (const C& c, I i)
    { return equal_n (begin(c), size(c), i); }

//}}}-------------------------------------------------------------------
//{{{ zstri

class zstri {
public:
    using value_type		= char;
    using pointer		= char*;
    using const_pointer		= const char*;
    using reference		= char&;
    using const_reference	= const char&;
    using difference_type	= unsigned;
public:
    constexpr		zstri (pointer s, difference_type n = UINT_MAX) NONNULL()
			    :_s(s),_n(n) {}
    template <difference_type N>
    constexpr		zstri (value_type (&a)[N]) :zstri(begin(a),N) {}
    constexpr		zstri (const zstri& i) = default;
    inline static auto	next (pointer s, difference_type& n) NONNULL() {
			    #if __x86__
			    if (!compile_constant(strlen(s)) || !compile_constant(n))
				__asm__("repnz scasb":"+D"(s),"+c"(n):"a"(0):"cc","memory");
			    else
			    #endif
			    { auto l = strnlen(s, n); l += !!l; s += l; n -= l; }
			    return s;
			}
    inline static auto	next (const_pointer s, difference_type& n) NONNULL()
			    { return const_cast<const_pointer> (zstri::next (const_cast<pointer>(s),n)); }
    inline static auto	next (pointer s) NONNULL()
			    { difference_type n = UINT_MAX; return next(s,n); }
    inline static auto	next (const_pointer s) NONNULL()
			    { difference_type n = UINT_MAX; return next(s,n); }
    constexpr auto	remaining (void) const	{ return _n; }
    constexpr auto	base (void) const	{ return _s; }
    constexpr auto&	operator* (void) const	{ return _s; }
    inline auto&	operator++ (void)	{ _s = next(_s,_n); return *this; }
    inline auto		operator++ (int)	{ auto o(*this); ++*this; return o; }
    inline auto&	operator+= (unsigned n)	{ while (n--) ++*this;  return *this; }
    inline auto		operator+ (unsigned n) const	{ auto r(*this); r += n; return r; }
    constexpr		operator bool (void) const	{ return remaining(); }
    constexpr bool	operator== (const zstri& i) const	{ return base() == i.base(); }
    constexpr bool	operator!= (const zstri& i) const	{ return base() != i.base(); }
    constexpr bool	operator< (const zstri& i) const	{ return base() < i.base(); }
    constexpr bool	operator<= (const zstri& i) const	{ return base() <= i.base(); }
    constexpr bool	operator> (const zstri& i) const	{ return i < *this; }
    constexpr bool	operator>= (const zstri& i) const	{ return i <= *this; }
private:
    pointer		_s;
    difference_type	_n;
};

class czstri {
public:
    using value_type		= zstri::value_type;
    using const_pointer		= zstri::const_pointer;
    using pointer		= const_pointer;
    using const_reference	= zstri::const_reference;
    using reference		= const_reference;
    using difference_type	= zstri::difference_type;
public:
    constexpr		czstri (pointer s, difference_type n = UINT_MAX) NONNULL()
			    :_s(s),_n(n) {}
    template <difference_type N>
    constexpr		czstri (const value_type (&a)[N]) : czstri (begin(a),N) {}
    constexpr		czstri (const zstri& i) : czstri (i.base(),i.remaining()) {}
    constexpr		czstri (const czstri& i) = default;
    inline static auto	next (pointer s, difference_type& n) NONNULL()
			    { return zstri::next (s,n); }
    constexpr auto	remaining (void) const	{ return _n; }
    constexpr auto	base (void) const	{ return _s; }
    constexpr auto&	operator* (void) const	{ return _s; }
    inline auto&	operator++ (void)	{ _s = next(_s,_n); return *this; }
    inline auto		operator++ (int)	{ auto o(*this); ++*this; return o; }
    inline auto&	operator+= (unsigned n)	{ while (n--) ++*this; return *this; }
    inline auto		operator+ (unsigned n) const	{ auto r(*this); r += n; return r; }
    constexpr		operator bool (void) const	{ return remaining(); }
    constexpr bool	operator== (const czstri& i) const	{ return base() == i.base(); }
    constexpr bool	operator!= (const czstri& i) const	{ return base() != i.base(); }
    constexpr bool	operator< (const czstri& i) const	{ return base() < i.base(); }
    constexpr bool	operator<= (const czstri& i) const	{ return base() <= i.base(); }
    constexpr bool	operator> (const czstri& i) const	{ return i < *this; }
    constexpr bool	operator>= (const czstri& i) const	{ return i <= *this; }
private:
    pointer		_s;
    difference_type	_n;
};

//}}}-------------------------------------------------------------------
//{{{ scope_exit

template <typename F>
class scope_exit {
public:
    constexpr explicit	scope_exit (F&& f) noexcept		: _f(move(f)),_enabled(true) {}
    constexpr		scope_exit (scope_exit&& f) noexcept	: _f(move(f._f)),_enabled(f._enabled) { f.release(); }
    constexpr void	release (void) noexcept			{ _enabled = false; }
    inline		~scope_exit (void) noexcept (noexcept (declval<F>()))	{ if (_enabled) _f(); }
    scope_exit&		operator= (scope_exit&&) = delete;
private:
    F		_f;
    bool	_enabled;
};

template <typename F>
constexpr auto make_scope_exit (F&& f) noexcept
    { return scope_exit<remove_reference_t<F>>(forward<F>(f)); }

//}}}-------------------------------------------------------------------
//{{{ Other algorithms

template <typename C, typename T>
void remove (C& c, const T& v)
{
    for (auto i = c.cbegin(); i < c.cend(); ++i)
	if (*i == v)
	    --(i = c.erase(i));
}

template <typename C, typename P>
void remove_if (C& c, P p)
{
    for (auto i = c.cbegin(); i < c.cend(); ++i)
	if (p(*i))
	    --(i = c.erase(i));
}

template <typename I>
void random_shuffle (I f, I l)
{
    for (; f < l; advance(f))
	iter_swap (f, next (f,(rand() % size_t(distance(f,l)))));
}
template <typename C>
inline void random_shuffle (C& c)
    { random_shuffle (begin(c), end(c)); }

template <typename I, typename T>
constexpr void iota (I f, I l, T v)
    { for (; f < l; advance(f),advance(v)) *f = v; }
template <typename C, typename T>
constexpr void iota (C& c, T v)
    { iota (begin(c), end(c), move(v)); }

template <typename I, typename T>
constexpr auto count (I f, I l, const T& v)
{
    auto r = 0u;
    for (; f < l; advance(f))
	if (*f == v)
	    ++r;
    return r;
}
template <typename C, typename T>
constexpr auto count (const C& c, const T& v)
    { return count (begin(c), end(c), v); }

template <typename I, typename P>
constexpr auto count_if (I f, I l, P p)
{
    auto r = 0u;
    for (; f < l; ++f)
	if (p(*f))
	    ++r;
    return r;
}
template <typename C, typename P>
constexpr auto count_if (const C& c, P p)
    { return count_if (begin(c), end(c), move(p)); }

//}}}-------------------------------------------------------------------
//{{{ C utility functions

// Read ntr bytes from fd, accounting for partial reads and EINTR
inline int complete_read (int fd, void* p, size_t ntr) noexcept
{
    int nr = 0;
    while (ntr) {
	auto r = read (fd, p, ntr);
	if (r <= 0) {
	    if (errno == EINTR)
		continue;
	    return -1;
	}
	ntr -= r;
	nr += r;
	advance (p, r);
    }
    return nr;
}

// Write ntw bytes to fd, accounting for partial writes and EINTR
inline int complete_write (int fd, const void* p, size_t ntw) noexcept
{
    int nw = 0;
    while (ntw) {
	auto r = write (fd, p, ntw);
	if (r <= 0) {
	    if (errno == EINTR)
		continue;
	    return -1;
	}
	ntw -= r;
	nw += r;
	advance (p, r);
    }
    return nw;
}

extern "C" {

#ifndef UC_VERSION
const char* executable_in_path (const char* efn, char* exe, size_t exesz) noexcept NONNULL();
int mkpath (const char* path, mode_t mode) noexcept NONNULL();
int rmpath (const char* path) noexcept NONNULL();
#endif

enum { SD_LISTEN_FDS_START = STDERR_FILENO+1 };
unsigned sd_listen_fds (void) noexcept;
int sd_listen_fd_by_name (const char* name) noexcept;

} // extern "C"
} // namespace cwiclo
//}}}-------------------------------------------------------------------
