// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "memory.h"

//{{{ zstr -------------------------------------------------------------
namespace cwiclo {

class zstr {
public:
    using value_type		= char;
    using pointer		= char*;
    using const_pointer		= const char*;
    using reference		= char&;
    using const_reference	= const char&;
    using difference_type	= unsigned;
    using index_type		= unsigned;
public:
    //{{{2 next, compare, nstrs, at, index -----------------------------

    inline static auto	next (pointer s, difference_type& n) NONNULL() {
	#if __x86__
	if constexpr (!compile_constant(strlen(s)) || !compile_constant(n))
	    __asm__("repnz scasb":"+D"(s),"+c"(n):"a"(0):"cc","memory");
	else
	#endif
	{ auto l = __builtin_strnlen(s, n); l += !!l; s += l; n -= l; }
	return s;
    }
    inline static auto	next (const_pointer s, difference_type& n) NONNULL()
	{ return const_cast<const_pointer> (zstr::next (const_cast<pointer>(s),n)); }
    inline static auto	next (pointer s) NONNULL()
	{ difference_type n = numeric_limits<difference_type>::max(); return next(s,n); }
    inline static auto	next (const_pointer s) NONNULL()
	{ difference_type n = numeric_limits<difference_type>::max(); return next(s,n); }
    inline static bool	compare (const void* s, const void* t, difference_type n) NONNULL() {
	#if __x86__ && !defined(__clang__)
	    bool e,l;
	    __asm__("repz cmpsb":"+D"(s),"+S"(t),"+c"(n),"=@cce"(e),"=@ccl"(l)::"memory");
	    return e;
	#else
	    return 0 == __builtin_memcmp (s, t, n);
	#endif
    }
    static index_type	nstrs (const_pointer p, difference_type n) NONNULL();
    template <difference_type N>
    static constexpr index_type	nstrs (const value_type (&a)[N]) {
			    index_type n = 0;
			    int ssz = N;
			    for (auto s = begin(a); ssz > 0; ++n) {
				auto sl = __builtin_strlen(s)+1;
				s += sl;
				ssz -= sl;
			    }
			    return n;
			}
    template <typename C>
    static index_type	nstrs (C& c)
			    { return nstrs (begin(c), size(c)); }
    static const_pointer at (index_type i, const_pointer p, difference_type n) NONNULL();
    static pointer	at (index_type i, pointer p, difference_type n) NONNULL()
			    { return const_cast<pointer> (at (i,const_cast<const_pointer>(p),n)); }
    template <typename I, difference_type N>
    static const_pointer at (I i, const value_type (&a)[N])
			    { return at (index_type(i), begin(a), size(a)); }
    template <typename I, typename C>
    static auto		at (I i, C& c)
			    { return at (index_type(i), begin(c), size(c)); }
    static index_type	index (const_pointer k, const_pointer p, difference_type n, index_type nf) NONNULL();
    template <typename I, difference_type N>
    static I		index (const_pointer k, const value_type (&a)[N], I nf)
			    { return I (index (k, begin(a), size(a), index_type(nf))); }

    //}}}2--------------------------------------------------------------
    //{{{2 ii - input iterator

    class ii {
    public:
	constexpr	ii (pointer s, difference_type n) NONNULL()
			:_s(s),_n(n) {}
	template <difference_type N>
	constexpr	ii (value_type (&a)[N]) :ii(begin(a),N) {}
	constexpr	ii (const ii& i) = default;
	constexpr auto	remaining (void) const	{ return _n; }
	constexpr auto	base (void) const	{ return _s; }
	constexpr auto&	operator* (void) const	{ return _s; }
	inline auto&	operator++ (void)	{ _s = next(_s,_n); return *this; }
	inline auto	operator++ (int)	{ auto o(*this); ++*this; return o; }
	inline auto&	operator+= (unsigned n)	{ while (n--) ++*this;  return *this; }
	inline auto	operator+ (unsigned n) const	{ auto r(*this); r += n; return r; }
	constexpr	operator bool (void) const	{ return remaining(); }
	constexpr bool	operator== (const ii& i) const	{ return base() == i.base(); }
	constexpr bool	operator!= (const ii& i) const	{ return base() != i.base(); }
	constexpr bool	operator< (const ii& i) const	{ return base() < i.base(); }
	constexpr bool	operator<= (const ii& i) const	{ return base() <= i.base(); }
	constexpr bool	operator> (const ii& i) const	{ return i < *this; }
	constexpr bool	operator>= (const ii& i) const	{ return i <= *this; }
    private:
	pointer		_s;
	difference_type	_n;
    };

    static constexpr auto in (pointer s, difference_type n) { return ii(s,n); }

    //}}}2--------------------------------------------------------------
    //{{{2 cii - const input iterator

    class cii {
    public:
	using pointer		= const_pointer;
	using reference		= const_reference;
    public:
	constexpr	cii (pointer s, difference_type n) NONNULL()
			    :_s(s),_n(n) {}
	template <difference_type N>
	constexpr	cii (const value_type (&a)[N]) : cii (begin(a),N) {}
	constexpr	cii (const ii& i) : cii (i.base(),i.remaining()) {}
	constexpr	cii (const cii& i) = default;
	constexpr auto	remaining (void) const	{ return _n; }
	constexpr auto	base (void) const	{ return _s; }
	constexpr auto&	operator* (void) const	{ return _s; }
	inline auto&	operator++ (void)	{ _s = next(_s,_n); return *this; }
	inline auto	operator++ (int)	{ auto o(*this); ++*this; return o; }
	inline auto&	operator+= (unsigned n)	{ while (n--) ++*this; return *this; }
	inline auto	operator+ (unsigned n) const	{ auto r(*this); r += n; return r; }
	constexpr	operator bool (void) const	{ return remaining(); }
	constexpr bool	operator== (const cii& i) const	{ return base() == i.base(); }
	constexpr bool	operator!= (const cii& i) const	{ return base() != i.base(); }
	constexpr bool	operator< (const cii& i) const	{ return base() < i.base(); }
	constexpr bool	operator<= (const cii& i) const	{ return base() <= i.base(); }
	constexpr bool	operator> (const cii& i) const	{ return i < *this; }
	constexpr bool	operator>= (const cii& i) const	{ return i <= *this; }
    private:
	pointer		_s;
	difference_type	_n;
    };

    static constexpr auto in (const_pointer s, difference_type n) { return cii(s,n); }

    //}}}2--------------------------------------------------------------
};

//}}}-------------------------------------------------------------------
//{{{ utf8
// stream iterators that read and write UTF-8 encoded characters.
// The encoding is defined as follows:
//
// U-00000000 - U-0000007F: 0xxxxxxx
// U-00000080 - U-000007FF: 110xxxxx 10xxxxxx
// U-00000800 - U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
// U-00010000 - U-001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
// U-00200000 - U-03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
// U-04000000 - U-7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
// U-80000000 - U-FFFFFFFF: 11111110 100000xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx

//----------------------------------------------------------------------

class utf8 {
public:
    //{{{2 Byte counters -----------------------------------------------

    // Returns the number of bytes in the character whose first char is c
    static constexpr auto ibytes (char c)
    {
	// Count the leading bits. Header bits are 1 * nBytes followed by a 0.
	//	0 - single byte character. Take 7 bits (0xFF >> 1)
	//	1 - error, in the middle of the character. Take 6 bits (0xFF >> 2)
	//	    so you will keep reading invalid entries until you hit the next character.
	//	>2 - multibyte character. Take remaining bits, and get the next bytes.
	//
	unsigned char mask = 0x80;
	unsigned n = 0;
	for (; c & mask; ++n)
	    mask >>= 1;
	return n+!n; // A sequence is always at least 1 byte.
    }

    /// Returns the number of bytes required to UTF-8 encode v.
    static constexpr auto obytes (char32_t v)
	{ return unsigned(v) < 0x80 ? 1u : (log2p1(v)+3)/5u; }

    static constexpr auto obytes (const char32_t* f, const char32_t* l)
    {
	auto n = 0u;
	for (; f < l; ++f)
	    n += obytes(*f);
	return n;
    }

    template <size_t N>
    static constexpr auto obytes (const char32_t (&a)[N])
	{ return obytes (begin(a), end(a)); }

    //}}}2--------------------------------------------------------------
    //{{{2 Input iterator

    template <typename I>
    class ii {
    public:
	using value_type	= typename iterator_traits<I>::value_type;
	using pointer		= typename iterator_traits<I>::pointer;
	using reference		= typename iterator_traits<I>::reference;
	using difference_type	= typename iterator_traits<I>::difference_type;
    public:
	explicit constexpr	ii (const I& i)		:_i(i) {}
	constexpr auto		base (void) const	{ return _i; }
	constexpr auto		operator* (void) const {
				    auto n = ibytes (*_i);
				    char32_t v = *_i & (0xff >> n);	// First byte contains bits after the header.
				    for (auto i = _i; --n && *++i;)	// Each subsequent byte has 6 bits.
					v = (v << 6) | (*i & 0x3f);
				    return v;
				}
	constexpr auto&		operator++ (void)	{ _i += ibytes(*_i); return *this; }
	constexpr auto		operator++ (int)	{ ii v (*this); operator++(); return v; }
	constexpr auto&		operator+= (unsigned n)	{ while (n--) operator++(); return *this; }
	constexpr auto		operator+ (unsigned n)	{ ii v (*this); return v += n; }
	constexpr bool		operator== (const ii& i) const	{ return _i == i._i; }
	constexpr bool		operator== (const I& i) const	{ return _i == i; }
	constexpr bool		operator!= (const ii& i) const	{ return _i != i._i; }
	constexpr bool		operator!= (const I& i) const	{ return _i != i; }
	constexpr bool		operator< (const ii& i) const	{ return _i < i._i; }
	constexpr bool		operator< (const I& i) const	{ return _i < i; }
	constexpr auto		operator- (const ii& i) const {
				    difference_type d = 0;
				    for (auto f (i); f < *this; ++f,++d) {}
				    return d;
				}
    private:
	I			_i;
    };

    template <typename I> static constexpr auto in (const I& i) { return ii<I>(i); }

    //}}}2--------------------------------------------------------------
    //{{{2 Output iterator

    template <typename O>
    class oi {
    public:
	using value_type	= typename iterator_traits<O>::value_type;
	using pointer		= typename iterator_traits<O>::pointer;
	using reference		= typename iterator_traits<O>::reference;
	using difference_type	= typename iterator_traits<O>::difference_type;
    public:
	explicit constexpr	oi (const O& o) : _o(o) {}
	constexpr auto&		operator= (char32_t v) {
				    auto n = obytes (v);
				    if (n <= 1) // If only one byte, there is no header.
					*_o++ = v;
				    else {	// Write the bits 6 bits at a time, except for the first one.
					char32_t btw = n * 6;
					*_o++ = ((v >> (btw -= 6)) & 0x3f) | (0xff << (8 - n));
					while (btw)
					    *_o++ = ((v >> (btw -= 6)) & 0x3f) | 0x80;
				    }
				    return *this;
				}
	constexpr auto&		base (void) const		{ return _o; }
	constexpr auto&		operator* (void)		{ return *this; }
	constexpr auto&		operator++ (void)		{ return *this; }
	constexpr auto&		operator++ (int)		{ return *this; }
	constexpr bool		operator== (const oi& o) const	{ return _o == o._o; }
	constexpr bool		operator== (const O& o) const	{ return _o == o; }
	constexpr bool		operator!= (const oi& o) const	{ return _o != o._o; }
	constexpr bool		operator!= (const O& o) const	{ return _o != o; }
	constexpr bool		operator< (const oi& o) const	{ return _o < o._o; }
	constexpr bool		operator< (const O& o) const	{ return _o < o; }
    private:
	O			_o;
    };

    template <typename O> static constexpr auto out (const O& o) { return oi<O>(o); }

    //}}}2--------------------------------------------------------------
};

//}}}-------------------------------------------------------------------
//{{{ scope_exit

template <typename F>
class scope_exit {
public:
    constexpr explicit	scope_exit (F&& f)		: _f(move(f)),_enabled(true) {}
    constexpr		scope_exit (scope_exit&& f)	: _f(move(f._f)),_enabled(f._enabled) { f.release(); }
    constexpr void	release (void)			{ _enabled = false; }
    inline		~scope_exit (void)		{ if (_enabled) _f(); }
    scope_exit&		operator= (scope_exit&&) = delete;
private:
    F		_f;
    bool	_enabled;
};

template <typename F>
constexpr auto make_scope_exit (F&& f)
    { return scope_exit<remove_reference_t<F>>(forward<F>(f)); }

//}}}-------------------------------------------------------------------
//{{{ Searching

template <typename I, typename T>
constexpr I find (I f, I l, const T& v)
{
    for (; f < l; advance(f))
	if (*f == v)
	    return f;
    return nullptr;
}

template <typename I, typename P>
constexpr I find_if (I f, I l, P p)
{
    for (; f < l; advance(f))
	if (p(*f))
	    return f;
    return nullptr;
}

template <typename I, typename P>
constexpr I find_if_not (I f, I l, P p)
{
    for (; f < l; advance(f))
	if (!p(*f))
	    return f;
    return nullptr;
}

template <typename I, typename J>
constexpr I find_first_of (I f1, I l1, J f2, J l2)
{
    for (; f1 < l1; ++f1)
	for (auto i = f2; i < l2; ++i)
	    if (*f1 == *i)
		return f1;
    return nullptr;
}

template <typename I, typename T>
constexpr auto lower_bound (I f, I l, const T& v)
{
    while (f < l) {
	auto m = midpoint(f,l);
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
	auto m = midpoint(f,l);
	if (v < *m)
	    l = m;
	else
	    f = next(m);
    }
    return l;
}

template <typename C, typename T>
inline constexpr auto find (C& c, const T& v)
    { return find (begin(c), end(c), v); }
template <typename C, typename P>
inline constexpr auto find_if (C& c, P p)
    { return find_if (begin(c), end(c), move(p)); }
template <typename C, typename P>
inline constexpr auto find_if_not (C& c, P p)
    { return find_if_not (begin(c), end(c), move(p)); }
template <typename C, typename T>
inline constexpr auto lower_bound (C& c, const T& v)
    { return lower_bound (begin(c), end(c), v); }
template <typename C, typename T>
inline constexpr auto upper_bound (C& c, const T& v)
    { return upper_bound (begin(c), end(c), v); }
template <typename C, typename T>
inline constexpr auto binary_search (C& c, const T& v)
    { return binary_search (begin(c), end(c), v); }

//}}}-------------------------------------------------------------------
//{{{ Comparisons

template <typename I1, typename I2>
bool equal_n (I1 i1, size_t n, I2 i2)
{
    using v1_type = make_unsigned_t<remove_const_t<typename iterator_traits<I1>::value_type>>;
    using v2_type = make_unsigned_t<remove_const_t<typename iterator_traits<I2>::value_type>>;
    if constexpr (is_trivially_assignable<v1_type>::value && is_same<v1_type,v2_type>::value)
	return zstr::compare (i1, i2, n *= sizeof(v1_type));
    while (n--)
	if (!(*i1++ == *i2++))
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

/// Returns true if the given range is sorted.
template <typename I>
constexpr bool is_sorted (I f, I l)
{
    for (auto i = f; ++i < l; ++f)
	if (*i < *f)
	    return false;
    return true;
}
template <typename C>
inline constexpr bool is_sorted (const C& c)
    { return is_sorted (begin(c), end(c)); }

//}}}-------------------------------------------------------------------
//{{{ Sorting

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

template <typename I>
constexpr void stable_sort (I f, I l)
{
    for (I j, i = f; ++i < l;) { // Insertion sort
	for (j = i; j-- > f && *i < *j;) {}
	if (++j != i) rotate (j, i, i + 1);
    }
}

template <typename C>
inline constexpr void sort (C& c)
    { sort (begin(c), end(c)); }
template <typename C>
inline constexpr void stable_sort (C& c)
    { stable_sort (begin(c), end(c)); }

//}}}-------------------------------------------------------------------
//{{{ Numerical

template <typename I, typename T>
constexpr auto accumulate (I f, I l, T v)
{
    for (; f < l; advance(f))
	v += *f;
    return v;
}

template <typename I, typename T, typename P>
constexpr auto accumulate (I f, I l, T v, P p)
{
    for (; f < l; advance(f))
	v += p(v,*f);
    return v;
}

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

template <typename I, typename T>
constexpr void iota (I f, I l, T v)
    { for (; f < l; advance(f),advance(v)) *f = v; }
template <typename C, typename T>
constexpr void iota (C& c, T v)
    { iota (begin(c), end(c), move(v)); }

template <typename I>
constexpr auto max_element (I f, I l)
{
    auto r = f;
    for (; f < l; advance(f))
	if (*r < *f)
	    r = f;
    return r;
}

template <typename I>
constexpr auto min_element (I f, I l)
{
    auto r = f;
    for (; f < l; advance(f))
	if (*f < *r)
	    r = f;
    return r;
}

//}}}-------------------------------------------------------------------
//{{{ remove, reverse, rotate_copy

template <typename C, typename T>
void remove (C& c, const T& v)
{
    foreach (i, c)
	if (*i == v)
	    --(i = c.erase(i));
}

template <typename C, typename P>
void remove_if (C& c, P p)
{
    foreach (i, c)
	if (p(*i))
	    --(i = c.erase(i));
}

template <typename I, typename O, typename T>
constexpr auto remove_copy (I f, I l, O r, const T& v)
{
    for (; f < l; advance(f))
	if (!(*f == v))
	    *r++ = *f;
    return r;
}
template <typename C, typename O, typename T>
inline constexpr auto remove_copy (const C& c, O r, const T& v)
    { return remove_copy (begin(c), end(c), r, v); }

template <typename I, typename O, typename RI>
constexpr auto remove_copy (I f, I l, O r, RI rf, RI rl)
{
    for (; f < l; advance(f))
	if (!find (rf, rl, *f))
	    *r++ = *f;
    return r;
}

template <typename I>
constexpr void reverse (I f, I l)
{
    while (f < --l)
	iter_swap (f++, l);
}
template <typename C>
inline constexpr void reverse (C& c)
    { reverse (begin(c), end(c)); }

template <typename I, typename O>
constexpr O reverse_copy (I f, I l, O r)
{
    while (f < l)
	*r++ = *--l;
    return r;
}
template <typename C, typename O>
inline constexpr auto reverse_copy (const C& c, O r)
    { return reverse_copy (begin(c), end(c), r); }

template <typename I, typename O>
inline O rotate_copy (I f, I m, I l, O r)
    { return copy (f, m, copy (m, l, r)); }

//}}}-------------------------------------------------------------------
//{{{ Merging

/// Combines two sorted ranges from the same container.
template <typename I>
constexpr void inplace_merge (I f, I m [[maybe_unused]], I l)
    { stable_sort (f, l); }

template <typename I1, typename I2, typename O>
O merge (I1 f1, I1 l1, I2 f2, I2 l2, O r)
{
    for (; f1 < l1 && f2 < l2; ++r) {
	if (*f1 < *f2)
	    *r = *f1++;
	else
	    *r = *f2++;
    }
    if (f1 < l1)
	return copy (f1, l1, r);
    else
	return copy (f2, l2, r);
}

template <typename I, typename O>
constexpr auto unique_copy (I f, I l, O r)
{
    if (f != l) {
	*r = *f;
	while (++f != l)
	    if (!(*f == *r))
		*++r = *f;
	++r;
    }
    return r;
}
template <typename C, typename O>
inline constexpr auto unique_copy (const C& c, O r)
    { return unique_copy (begin(c), end(c), r); }

template <typename I>
inline constexpr auto unique (I f, I l)
    { return unique_copy (f, l, f); }
template <typename C>
inline void unique (C& c)
    { c.erase (unique (c.begin(), c.end()), c.end()); }

//}}}-------------------------------------------------------------------
//{{{ Transformation

template <typename I, typename G>
constexpr void generate_n (I f, size_t n, G g)
{
    for (; n--; advance(f))
	*f = g();
}

template <typename I, typename G>
constexpr void generate (I f, I l, G g)
{
    for (; f < l; advance(f))
	*f = g();
}
template <typename C, typename G>
constexpr auto generate (C& c, G g)
    { return generate (begin(c), end(c), move(g)); }

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
constexpr void replace (I f, I l, const T& ov, const T& nv)
{
    for (; f < l; advance(f))
	if (*f == ov)
	    *f = nv;
}
template <typename C, typename T>
inline constexpr void replace (C& c, const T& ov, const T& nv)
    { replace (begin(c), end(c), ov, nv); }

template <typename I, typename O, typename T>
constexpr auto replace_copy (I f, I l, O r, const T& ov, const T& nv)
{
    for (; f < l; advance(f), advance(r))
	*r = (*f == ov ? nv : *f);
    return r;
}
template <typename C, typename O, typename T>
inline constexpr auto replace_copy (const C& c, O r, const T& ov, const T& nv)
    { return replace_copy (begin(c), end(c), r, ov, nv); }

//}}}-------------------------------------------------------------------
//{{{ C utility functions

namespace {

// Read ntr bytes from fd, accounting for partial reads and EINTR
inline static int complete_read (int fd, void* p, size_t ntr)
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
inline static int complete_write (int fd, const void* p, size_t ntw)
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

// Number printing without printf, which can undesirably load locales
template <unsigned N>
static constexpr char* uint_to_text (unsigned n, char (&a)[N])
{
    auto t = end(a);
    *--t = 0;
    for (; n; n /= 10)
	*--t = '0'+(n%10);
    return t;
}

} // namespace

//----------------------------------------------------------------------

} // namespace cwiclo
extern "C" {

#ifdef __linux__
void closefrom (int fd);
#endif

#ifndef UC_VERSION
int mkpath (const char* path, mode_t mode) NONNULL();
int rmpath (const char* path) NONNULL();

enum { SD_LISTEN_FDS_START = STDERR_FILENO+1 };
unsigned sd_listen_fds (void);
int sd_listen_fd_by_name (const char* name);
#endif

#ifndef NDEBUG
const char* debug_socket_name (const struct sockaddr* addr);
#endif

int create_sockaddr_un (struct sockaddr_un* addr, const char* fmt, const char* path, const char* sockname);

inline int create_sockaddr_local (struct sockaddr_un* addr, const char* sockname)
    { return create_sockaddr_un (addr, "%s%s", "", sockname); }
inline int create_sockaddr_system_local (struct sockaddr_un* addr, const char* sockname)
    { return create_sockaddr_un (addr, "%s/%s", "/run", sockname); }
inline int create_sockaddr_user_local (struct sockaddr_un* addr, const char* sockname)
{
    auto rundir = getenv ("XDG_RUNTIME_DIR");
    return create_sockaddr_un (addr, "%s/%s", rundir ? rundir : "/tmp", sockname);
}

} // extern "C"
//}}}-------------------------------------------------------------------
