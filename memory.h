// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "utility.h"

//{{{ initializer_list -------------------------------------------------
namespace std {	// replaced gcc internal stuff must be in std::

/// Internal class for compiler support of C++11 initializer lists
template <typename T>
class initializer_list {
public:
    using value_type		= T;
    using size_type		= size_t;
    using const_reference	= const value_type&;
    using reference		= const_reference;
    using const_iterator	= const value_type*;
    using iterator		= const_iterator;
private:
    // This object can only be constructed by the compiler when the
    // {1,2,3} syntax is used, so the constructor must be private
    inline constexpr		initializer_list (const_iterator p, size_type sz) noexcept : _data(p), _size(sz) {}
public:
    inline constexpr		initializer_list (void)noexcept	: _data(nullptr), _size(0) {}
    inline constexpr auto	size (void) const noexcept	{ return _size; }
    inline constexpr auto	begin (void) const noexcept	{ return _data; }
    inline constexpr auto	end (void) const noexcept	{ return begin()+size(); }
    inline constexpr auto&	operator[] (size_type i) const	{ return begin()[i]; }
private:
    iterator			_data;
    size_type			_size;
};

} // namespace std
//}}}-------------------------------------------------------------------
//{{{ new and delete

extern "C" [[nodiscard]] void* _realloc (void* p, size_t n) noexcept MALLOCLIKE MALLOCLIKE_ARG(2);
extern "C" [[nodiscard]] void* _alloc (size_t n) noexcept MALLOCLIKE MALLOCLIKE_ARG(1);

#if __clang__
// clang may have reasons to want a delete symbol, but in cwiclo all
// calls to new/delete must go to malloc/free anyway and no custom
// allocators are supported. So just make the warning go away.
#pragma GCC diagnostic ignored "-Winline-new-delete"
#endif

[[nodiscard]] inline void* operator new (size_t n)	{ return _alloc(n); }
[[nodiscard]] inline void* operator new[] (size_t n)	{ return _alloc(n); }
inline void  operator delete (void* p) noexcept		{ free(p); }
inline void  operator delete[] (void* p) noexcept	{ free(p); }
inline void  operator delete (void* p, size_t) noexcept	{ free(p); }
inline void  operator delete[] (void* p, size_t) noexcept { free(p); }

// Default placement versions of operator new.
[[nodiscard]] inline void* operator new (size_t, void* p)	{ return p; }
[[nodiscard]] inline void* operator new[] (size_t, void* p)	{ return p; }

// Default placement versions of operator delete.
inline void  operator delete  (void*, void*)	{ }
inline void  operator delete[](void*, void*)	{ }

//}}}-------------------------------------------------------------------
//{{{ Additional C++ ABI support

namespace std {

[[noreturn]] void terminate (void) noexcept;

} // namespace std
namespace __cxxabiv1 {

using __guard = cwiclo::atomic_flag;

extern "C" {

[[noreturn]] void __cxa_bad_cast (void) noexcept;
[[noreturn]] void __cxa_bad_typeid (void) noexcept;
[[noreturn]] void __cxa_throw_bad_array_new_length (void) noexcept;
[[noreturn]] void __cxa_throw_bad_array_length (void) noexcept;

// Compiler-generated thread-safe statics initialization
int __cxa_guard_acquire (__guard* g) noexcept;
void __cxa_guard_release (__guard* g) noexcept;
void __cxa_guard_abort (__guard* g) noexcept;

} // extern "C"
} // namespace __cxxabiv1
namespace abi = __cxxabiv1;

//}}}-------------------------------------------------------------------
//{{{ Rvalue forwarding

namespace cwiclo {

template <typename T> [[nodiscard]] constexpr decltype(auto)
forward (remove_reference_t<T>& v) noexcept { return static_cast<T&&>(v); }

template<typename T> [[nodiscard]] constexpr decltype(auto)
forward (remove_reference_t<T>&& v) noexcept { return static_cast<T&&>(v); }

template<typename T> [[nodiscard]] constexpr decltype(auto)
move(T&& v) noexcept { return static_cast<remove_reference_t<T>&&>(v); }

//}}}-------------------------------------------------------------------
//{{{ Swap algorithms

/// Assigns the contents of a to b and the contents of b to a.
/// This is used as a primitive operation by many other algorithms.
template <typename T>
inline void swap (T& a, T& b)
    { auto t = move(a); a = move(b); b = move(t); }

template <typename T, size_t N>
inline void swap (T (&a)[N], T (&b)[N])
{
    for (size_t i = 0; i < N; ++i)
	swap (a[i], b[i]);
}

template <typename T, typename U = T>
inline constexpr auto exchange (T& o, U&& nv)
{
    auto ov = move(o);
    o = forward<T>(nv);
    return ov;
}

template <typename I>
inline void iter_swap (I a, I b)
    { swap (*a, *b); }

//}}}-------------------------------------------------------------------
//{{{ iterator_traits
template <typename Iterator>
struct iterator_traits {
    using value_type		= typename Iterator::value_type;
    using difference_type	= typename Iterator::difference_type;
    using pointer		= typename Iterator::pointer;
    using reference		= typename Iterator::reference;
};

template <typename T>
struct iterator_traits<T*> {
    using value_type		= T;
    using difference_type	= ptrdiff_t;
    using const_pointer		= const value_type*;
    using pointer		= value_type*;
    using const_reference	= const value_type&;
    using reference		= value_type&;
};
template <typename T>
struct iterator_traits<const T*> {
    using value_type		= T;
    using difference_type	= ptrdiff_t;
    using const_pointer		= const value_type*;
    using pointer		= const_pointer;
    using const_reference	= const value_type&;
    using reference		= const_reference;
};
template <>
struct iterator_traits<void*> {
    using value_type		= char;
    using difference_type	= ptrdiff_t;
    using const_pointer		= const void*;
    using pointer		= void*;
    using const_reference	= const value_type&;
    using reference		= value_type&;
};
template <>
struct iterator_traits<const void*> {
    using value_type		= char;
    using difference_type	= ptrdiff_t;
    using const_pointer		= const void*;
    using pointer		= const_pointer;
    using const_reference	= const value_type&;
    using reference		= const_reference;
};

//}}}-------------------------------------------------------------------
//{{{ unique_ptr

/// \brief A smart pointer.
///
/// Calls delete in the destructor; assignment transfers ownership.
/// This class does not work with void pointers due to the absence
/// of the required dereference operator.
template <typename T>
class unique_ptr {
public:
    using element_type		= T;
    using pointer		= element_type*;
    using reference		= element_type&;
public:
    inline constexpr		unique_ptr (void)		: _p (nullptr) {}
    inline constexpr explicit	unique_ptr (pointer p)		: _p (p) {}
    inline			unique_ptr (unique_ptr&& p)	: _p (p.release()) {}
    inline			~unique_ptr (void)		{ delete _p; }
    inline constexpr auto	get (void) const		{ return _p; }
    inline auto			release (void)			{ return exchange (_p, nullptr); }
    inline void			reset (pointer p = nullptr)	{ assert (p != _p || !p); delete exchange (_p, p); }
    inline void			swap (unique_ptr&& v)		{ ::cwiclo::swap (_p, v._p); }
    inline constexpr explicit	operator bool (void) const	{ return _p != nullptr; }
    inline auto&		operator= (pointer p)		{ reset (p); return *this; }
    inline auto&		operator= (unique_ptr&& p)	{ reset (p.release()); return *this; }
    inline constexpr auto&	operator* (void) const		{ return *get(); }
    inline constexpr auto	operator-> (void) const		{ return get(); }
    inline constexpr bool	operator== (const pointer p) const	{ return _p == p; }
    inline constexpr bool	operator== (const unique_ptr& p) const	{ return _p == p._p; }
    inline constexpr bool	operator< (const pointer p) const	{ return _p < p; }
    inline constexpr bool	operator< (const unique_ptr& p) const	{ return _p < p._p; }
private:
    pointer			_p;
};

template <typename T, typename... Args>
inline auto make_unique (Args&&... args) { return unique_ptr<T> (new T (forward<Args>(args)...)); }

//}}}-------------------------------------------------------------------
//{{{ scope_exit

template <typename F>
class scope_exit {
public:
    inline explicit	scope_exit (F&& f) noexcept		: _f(move(f)),_enabled(true) {}
    inline		scope_exit (scope_exit&& f) noexcept	: _f(move(f._f)),_enabled(f._enabled) { f.release(); }
    inline void		release (void) noexcept			{ _enabled = false; }
    inline		~scope_exit (void) noexcept (noexcept (declval<F>()))	{ if (_enabled) _f(); }
    scope_exit&		operator= (scope_exit&&) = delete;
private:
    F		_f;
    bool	_enabled;
};

template <typename F>
auto make_scope_exit (F&& f) noexcept
    { return scope_exit<remove_reference_t<F>>(forward<F>(f)); }

extern "C" void print_backtrace (void) noexcept;
#ifndef UC_VERSION
extern "C" void hexdump (const void* vp, size_t n) noexcept;
#endif

//}}}-------------------------------------------------------------------
//{{{ construct and destroy

/// Calls the placement new on \p p.
template <typename T>
inline auto construct_at (T* p)
    { return new (p) T; }

/// Calls the placement new on \p p.
template <typename T>
inline auto construct_at (T* p, const T& value)
    { return new (p) T (value); }

/// Calls the placement new on \p p with \p args.
template <typename T, typename... Args>
inline auto construct_at (T* p, Args&&... args)
    { return new (p) T (forward<Args>(args)...); }

/// Calls the destructor of \p p without calling delete.
template <typename T>
inline void destroy_at (T* p) noexcept
    { p->~T(); }

/// Calls the placement new on \p p.
template <typename I>
auto uninitialized_default_construct (I f, I l)
{
    if constexpr (is_trivially_constructible<typename iterator_traits<I>::value_type>::value) {
	if (f < l)
	    memset (reinterpret_cast<void*>(f), 0, (l-f)*sizeof(*f));
    } else for (; f < l; ++f)
	construct_at (f);
    return f;
}

/// Calls the placement new on \p p.
template <typename I>
auto uninitialized_default_construct_n (I f, size_t n)
    { return uninitialized_default_construct (f, f+n); }

/// Calls the destructor on elements in range [f, l) without calling delete.
template <typename I>
void destroy (I f [[maybe_unused]], I l [[maybe_unused]]) noexcept
{
    if constexpr (!is_trivially_destructible<typename iterator_traits<I>::value_type>::value) {
	for (; f < l; ++f)
	    destroy_at (f);
    }
#ifndef NDEBUG
    else if (f < l)
	memset (reinterpret_cast<void*>(f), 0xcd, (l-f)*sizeof(*f));
#endif
}

/// Calls the destructor on elements in range [f, f+n) without calling delete.
template <typename I>
inline void destroy_n (I f, size_t n) noexcept
    { return destroy (f, f+n); }

//}}}-------------------------------------------------------------------
//{{{ copy and fill

template <typename II, typename OI>
auto copy_n (II f, size_t n, OI r)
{
    using ivalue_type = remove_inner_const_t<typename iterator_traits<II>::value_type>;
    using ovalue_type = remove_inner_const_t<typename iterator_traits<OI>::value_type>;
    if constexpr (is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value) {
#if __x86__
	if constexpr (compile_constant(n))
#endif
	    return OI (__builtin_mempcpy (r, f, n*sizeof(ovalue_type)));
#if __x86__
#if __x86_64__
	else if constexpr (!(sizeof(ovalue_type)%8)) {
	    n *= sizeof(ovalue_type)/8;
	    __asm__ volatile ("rep movsq":"+S"(f),"+D"(r),"+c"(n)::"memory","cc");
	}
#endif
	else if constexpr (!(sizeof(ovalue_type)%4)) {
	    n *= sizeof(ovalue_type)/4;
	    __asm__ volatile ("rep movsl":"+S"(f),"+D"(r),"+c"(n)::"memory","cc");
	} else if constexpr (!(sizeof(ovalue_type)%2)) {
	    n *= sizeof(ovalue_type)/2;
	    __asm__ volatile ("rep movsw":"+S"(f),"+D"(r),"+c"(n)::"memory","cc");
	} else {
	    n *= sizeof(ovalue_type);
	     __asm__ volatile ("rep movsb":"+S"(f),"+D"(r),"+c"(n)::"memory","cc");
	}
#endif
    } else for (auto l = f+n; f < l; ++r, ++f)
	*r = *f;
    return r;
}

template <typename II, typename OI>
auto copy (II f, II l, OI r)
{
    using ivalue_type = remove_inner_const_t<typename iterator_traits<II>::value_type>;
    using ovalue_type = remove_inner_const_t<typename iterator_traits<OI>::value_type>;
    if constexpr (is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return copy_n (f, l-f, r);
    for (; f < l; ++r, ++f)
	*r = *f;
    return r;
}

template <typename II, typename OI>
auto copy_backward_n (II f, size_t n, OI r)
{
    using ivalue_type = remove_inner_const_t<typename iterator_traits<II>::value_type>;
    using ovalue_type = remove_inner_const_t<typename iterator_traits<OI>::value_type>;
    if constexpr (is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value) {
#if __x86__
	const char* cf = reinterpret_cast<const char*>(&*(f+n));
	char* cr = reinterpret_cast<char*>(&*(r+n));
#if __x86_64__
	if constexpr (!(sizeof(ovalue_type)%8)) {
	    n *= sizeof(ovalue_type)/8; cf -= 8; cr -= 8;
	    __asm__ volatile ("std\n\trep movsq\n\tcld":"+S"(cf),"+D"(cr),"+c"(n)::"memory","cc");
	} else
#endif
	if constexpr (!(sizeof(ovalue_type)%4)) {
	    n *= sizeof(ovalue_type)/4; cf -= 4; cr -= 4;
	    __asm__ volatile ("std\n\trep movsl\n\tcld":"+S"(cf),"+D"(cr),"+c"(n)::"memory","cc");
	} else if constexpr (!(sizeof(ovalue_type)%2)) {
	    n *= sizeof(ovalue_type)/2; cf -= 2; cr -= 2;
	    __asm__ volatile ("std\n\trep movsw\n\tcld":"+S"(cf),"+D"(cr),"+c"(n)::"memory","cc");
	} else {
	    n *= sizeof(ovalue_type); --cf; --cr;
	     __asm__ volatile ("std\n\trep movsb\n\tcld":"+S"(cf),"+D"(cr),"+c"(n)::"memory","cc");
	}
	return OI(cr);
#else // !__x86__
	return memmove (r, f, n*sizeof(ovalue_type));
#endif
    }
    while (n--)
	r[n] = f[n];
    return r;
}

template <typename II, typename OI>
auto copy_backward (II f, II l, OI r)
{
    using ivalue_type = remove_inner_const_t<typename iterator_traits<II>::value_type>;
    using ovalue_type = remove_inner_const_t<typename iterator_traits<OI>::value_type>;
    if constexpr (is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return copy_backward_n (f, l-f, r);
    while (f < l)
	*--r = *--l;
    return r;
}

template <typename I, typename T>
auto fill_n (I f, size_t n, const T& v)
{
    using ivalue_type = remove_inner_const_t<T>;
    using ovalue_type = remove_inner_const_t<typename iterator_traits<I>::value_type>;
    constexpr bool canstos = is_trivial<ovalue_type>::value && is_same<ivalue_type,ovalue_type>::value;
    if constexpr (canstos && sizeof(ovalue_type) == 1)
#if __x86__
	__asm__ volatile ("rep\tstosb":"+D"(f),"+c"(n):"a"(union_cast<uint8_t>(v)):"memory","cc");
    else if constexpr (canstos && sizeof(ovalue_type) == 2)
	__asm__ volatile ("rep\tstosw":"+D"(f),"+c"(n):"a"(union_cast<uint16_t>(v)):"memory","cc");
    else if constexpr (canstos && sizeof(ovalue_type) == 4)
	__asm__ volatile ("rep\tstosl":"+D"(f),"+c"(n):"a"(union_cast<uint32_t>(v)):"memory","cc");
#if __x86_64__
    else if constexpr (canstos && sizeof(ovalue_type) == 8)
	__asm__ volatile ("rep\tstosq":"+D"(f),"+c"(n):"a"(union_cast<uint64_t>(v)):"memory","cc");
#endif
#else // !__x86__, 1 byte fill
	{ memset (f, union_cast<uint8_t>(v), n); f += n; }
#endif
    else for (auto l = f+n; f < l; ++f)
	*f = v;
    return f;
}

template <typename I, typename T>
auto fill (I f, I l, const T& v)
{
    using ivalue_type = remove_inner_const_t<T>;
    using ovalue_type = remove_inner_const_t<typename iterator_traits<I>::value_type>;
    if constexpr (is_trivial<ovalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return fill_n (f, l-f, v);
    for (; f < l; ++f)
	*f = v;
    return f;
}

extern "C" void brotate (void* vf, void* vm, void* vl) noexcept;

template <typename T>
T* rotate (T* f, T* m, T* l)
    { brotate (f, m, l); return f; }

//}}}-------------------------------------------------------------------
//{{{ uninitialized fill and copy

/// Copies [f, l) into r by calling copy constructors in r.
template <typename II, typename OI>
inline auto uninitialized_copy (II f, II l, OI r)
{
    using ivalue_type = remove_inner_const_t<typename iterator_traits<II>::value_type>;
    using ovalue_type = remove_inner_const_t<typename iterator_traits<OI>::value_type>;
    if constexpr (is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return copy (f, l, r);
    for (; f < l; ++r, ++f)
	construct_at (&*r, *f);
    return r;
}

/// Copies [f, f + n) into r by calling copy constructors in r.
template <typename II, typename OI>
inline auto uninitialized_copy_n (II f, size_t n, OI r)
{
    using ivalue_type = remove_inner_const_t<typename iterator_traits<II>::value_type>;
    using ovalue_type = remove_inner_const_t<typename iterator_traits<OI>::value_type>;
    if constexpr (is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return copy_n (f, n, r);
    for (auto i = 0u; i < n; ++i)
	construct_at (&r[i], f[i]);
    return r;
}

/// Copies [f, l) into r by calling move constructors in r.
template <typename II, typename OI>
inline auto uninitialized_move (II f, II l, OI r)
{
    using ivalue_type = remove_inner_const_t<typename iterator_traits<II>::value_type>;
    using ovalue_type = remove_inner_const_t<typename iterator_traits<OI>::value_type>;
    if constexpr (is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return copy (f, l, r);
    for (; f < l; ++r, ++f)
	construct_at (&*r, move(*f));
    return r;
}

/// Copies [f, f + n) into r by calling move constructors in r.
template <typename II, typename OI>
inline auto uninitialized_move_n (II f, size_t n, OI r)
{
    using ivalue_type = remove_inner_const_t<typename iterator_traits<II>::value_type>;
    using ovalue_type = remove_inner_const_t<typename iterator_traits<OI>::value_type>;
    if constexpr (is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return copy_n (f, n, r);
    for (auto i = 0u; i < n; ++i)
	construct_at (&r[i], move(f[i]));
    return r;
}

/// Calls construct on all elements in [f, l) with value \p v.
template <typename I, typename T>
inline void uninitialized_fill (I f, I l, const T& v)
{
    using ivalue_type = remove_inner_const_t<T>;
    using ovalue_type = remove_inner_const_t<typename iterator_traits<I>::value_type>;
    if constexpr (is_trivially_constructible<ovalue_type>::value && is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	fill (f, l, v);
    else for (; f < l; ++f)
	construct_at (&*f, v);
}

/// Calls construct on all elements in [f, f + n) with value \p v.
template <typename I, typename T>
inline auto uninitialized_fill_n (I f, size_t n, const T& v)
{
    using ivalue_type = remove_inner_const_t<T>;
    using ovalue_type = remove_inner_const_t<typename iterator_traits<I>::value_type>;
    if constexpr (is_trivially_constructible<ovalue_type>::value && is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	fill_n (f, n, v);
    else for (auto i = 0u; i < n; ++i)
	construct_at (&f[i], v);
    return f;
}

//}}}-------------------------------------------------------------------
//{{{ Searching algorithms

template <typename I, typename T>
I linear_search (I f, I l, const T& v)
{
    for (; f < l; ++f)
	if (*f == v)
	    return f;
    return nullptr;
}

template <typename I, typename P>
I linear_search_if (I f, I l, P p)
{
    for (; f < l; ++f)
	if (p(*f))
	    return f;
    return nullptr;
}

template <typename I, typename T>
auto lower_bound (I f, I l, const T& v)
{
    while (f < l) {
	auto m = f + uintptr_t(l - f)/2;
	if (*m < v)
	    f = m + 1;
	else
	    l = m;
    }
    return f;
}

template <typename I, typename T>
inline auto binary_search (I f, I l, const T& v)
{
    auto b = lower_bound (f, l, v);
    return (b < l && *b == v) ? b : nullptr;
}

template <typename I, typename T>
auto upper_bound (I f, I l, const T& v)
{
    while (f < l) {
	auto m = f + uintptr_t(l - f)/2;
	if (v < *m)
	    l = m;
	else
	    f = m + 1;
    }
    return l;
}

template <typename T>
int c_compare (const void* p1, const void* p2)
{
    auto& v1 = *static_cast<const T*>(p1);
    auto& v2 = *static_cast<const T*>(p2);
    return v1 < v2 ? -1 : (v2 < v1 ? 1 : 0);
}

template <typename I>
void sort (I f, I l)
{
    using value_type = typename iterator_traits<I>::value_type;
    qsort (f, l-f, sizeof(value_type), c_compare<value_type>);
}

template <typename Ctr>
auto linear_search (Ctr& c, typename Ctr::const_reference v)
    { return linear_search (c.begin(), c.end(), v); }
template <typename Ctr, typename P>
auto linear_search_if (Ctr& c, P p)
    { return linear_search_if (c.begin(), c.end(), p); }
template <typename Ctr>
auto lower_bound (Ctr& c, typename Ctr::const_reference v)
    { return lower_bound (c.begin(), c.end(), v); }
template <typename Ctr>
auto upper_bound (Ctr& c, typename Ctr::const_reference v)
    { return upper_bound (c.begin(), c.end(), v); }
template <typename Ctr>
auto binary_search (Ctr& c, typename Ctr::const_reference v)
    { return binary_search (c.begin(), c.end(), v); }
template <typename Ctr>
void sort (Ctr& c)
    { sort (c.begin(), c.end()); }

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
    inline explicit	zstri (pointer s, difference_type n = UINT_MAX) NONNULL()
			    :_s(s),_n(n) {}
    template <difference_type N>
    inline explicit	zstri (value_type (&a)[N]) :zstri(begin(a),N) {}
			zstri (const zstri& i) = default;
    inline static auto	next (pointer s, difference_type& n) NONNULL() {
			    #if __x86__
			    if (!compile_constant(strlen(s)) || !compile_constant(n))
				__asm__("repnz\tscasb":"+D"(s),"+c"(n):"a"(0):"cc","memory");
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
    inline auto		remaining (void) const	{ return _n; }
    inline auto		base (void) const	{ return _s; }
    inline auto&	operator* (void) const	{ return _s; }
    inline auto&	operator++ (void)	{ _s = next(_s,_n); return *this; }
    inline auto		operator++ (int)	{ auto o(*this); ++*this; return o; }
    inline auto&	operator+= (unsigned n)	{ while (n--) ++*this;  return *this; }
    inline auto		operator+ (unsigned n) const	{ auto r(*this); r += n; return r; }
    inline		operator bool (void) const	{ return remaining(); }
    inline bool		operator== (const zstri& i) const	{ return base() == i.base(); }
    inline bool		operator< (const zstri& i) const	{ return base() < i.base(); }
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
    inline explicit	czstri (pointer s, difference_type n = UINT_MAX) NONNULL()
			    :_s(s),_n(n) {}
    template <difference_type N>
    inline explicit	czstri (const value_type (&a)[N]) : czstri (begin(a),N) {}
    inline explicit	czstri (const zstri& i) : czstri (i.base(),i.remaining()) {}
			czstri (const czstri& i) = default;
    inline static auto	next (pointer s, difference_type& n) NONNULL()
			    { return zstri::next (s,n); }
    inline auto		remaining (void) const	{ return _n; }
    inline auto		base (void) const	{ return _s; }
    inline auto&	operator* (void) const	{ return _s; }
    inline auto&	operator++ (void)	{ _s = next(_s,_n); return *this; }
    inline auto		operator++ (int)	{ auto o(*this); ++*this; return o; }
    inline auto&	operator+= (unsigned n)	{ while (n--) ++*this; return *this; }
    inline auto		operator+ (unsigned n) const	{ auto r(*this); r += n; return r; }
    inline		operator bool (void) const	{ return remaining(); }
    inline bool		operator== (const czstri& i) const	{ return base() == i.base(); }
    inline bool		operator< (const czstri& i) const	{ return base() < i.base(); }
private:
    pointer		_s;
    difference_type	_n;
};

//}}}-------------------------------------------------------------------
//{{{ Other algorithms

template <typename Ctr, typename Discriminator>
void remove_if (Ctr& ctr, Discriminator f)
{
    for (auto i = ctr.cbegin(); i < ctr.cend(); ++i)
	if (f(*i))
	    --(i = ctr.erase(i));
}

template <typename I>
void random_shuffle (I f, I l)
{
    for (; f < l; ++f)
	iter_swap (f, f + (rand() % size_t(l-f)));
}
template <typename Ctr>
inline void random_shuffle (Ctr& ctr)
    { random_shuffle (ctr.begin(), ctr.end()); }

template <typename I>
inline void iota (I f, I l, typename iterator_traits<I>::value_type v)
{
    for (; f < l; ++f,++v)
	*f = v;
}

} // namespace cwiclo
//}}}-------------------------------------------------------------------
