// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "iterator.h"

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
    constexpr		initializer_list (const_iterator p, size_type sz) : _data(p), _size(sz) {}
public:
    constexpr		initializer_list (void)	: _data(nullptr), _size(0) {}
    constexpr auto	size (void) const	{ return _size; }
    constexpr auto	begin (void) const	{ return _data; }
    constexpr auto	end (void) const	{ return begin()+size(); }
    constexpr auto&	operator[] (size_type i) const	{ return begin()[i]; }
private:
    iterator		_data;
    size_type		_size;
};

//}}}-------------------------------------------------------------------
//{{{ strong_ordering

// For <=>
class strong_ordering {
    int	_v;
public:
    constexpr strong_ordering (int v) :_v(v) {}
    constexpr operator int (void) const { return _v; }
    constexpr bool operator== (int v) const { return _v == v; }
    constexpr bool operator< (int v) const { return _v < v; }
    constexpr bool operator<= (int v) const { return _v <= v; }
    constexpr bool operator> (int v) const { return _v > v; }
    constexpr bool operator>= (int v) const { return _v >= v; }
    constexpr bool operator== (const strong_ordering& v) const { return _v == v._v; }
    constexpr bool operator< (const strong_ordering& v) const { return _v < v._v; }
    constexpr bool operator<= (const strong_ordering& v) const { return _v <= v._v; }
    constexpr bool operator> (const strong_ordering& v) const { return _v > v._v; }
    constexpr bool operator>= (const strong_ordering& v) const { return _v >= v._v; }
    static const strong_ordering less;
    static const strong_ordering equal;
    static const strong_ordering greater;
};

__inline constexpr const strong_ordering strong_ordering::less (-1);
__inline constexpr const strong_ordering strong_ordering::equal (0);
__inline constexpr const strong_ordering strong_ordering::greater (1);

} // namespace std
//}}}-------------------------------------------------------------------
//{{{ new and delete

extern "C" [[nodiscard]] void* _realloc (void* p, size_t n) MALLOCLIKE MALLOCLIKE_ARG(2);
extern "C" [[nodiscard]] void* _alloc (size_t n) MALLOCLIKE MALLOCLIKE_ARG(1);

#if __clang__
// clang may have reasons to want a delete symbol, but in cwiclo all
// calls to new/delete must go to malloc/free anyway and no custom
// allocators are supported. So just make the warning go away.
#pragma GCC diagnostic ignored "-Winline-new-delete"
#endif

[[nodiscard]] inline void* operator new (size_t n)	{ return _alloc(n); }
[[nodiscard]] inline void* operator new[] (size_t n)	{ return _alloc(n); }
inline void operator delete (void* p)			{ free(p); }
inline void operator delete[] (void* p)			{ free(p); }
inline void operator delete (void* p, size_t)		{ free(p); }
inline void operator delete[] (void* p, size_t)		{ free(p); }

// Default placement versions of operator new.
[[nodiscard]] constexpr void* operator new (size_t, void* p)	{ return p; }
[[nodiscard]] constexpr void* operator new[] (size_t, void* p)	{ return p; }

// Default placement versions of operator delete.
inline void operator delete  (void*, void*)	{ }
inline void operator delete[](void*, void*)	{ }

//}}}-------------------------------------------------------------------
//{{{ unique_ptr
namespace cwiclo {

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
    inline constexpr		unique_ptr (unique_ptr&& p)	: _p (p.release()) {}
    inline			~unique_ptr (void)		{ reset(); }
    inline constexpr auto	get (void) const		{ return _p; }
    inline constexpr auto	release (void)			{ return exchange (_p, nullptr); }
    inline constexpr void	reset (pointer p = nullptr)	{ assert (p != _p || !p); delete exchange (_p, p); }
    inline constexpr void	swap (unique_ptr& v)		{ ::cwiclo::swap (_p, v._p); }
    inline constexpr explicit	operator bool (void) const	{ return _p != nullptr; }
    inline constexpr auto&	operator= (pointer p)		{ reset (p); return *this; }
    inline constexpr auto&	operator= (unique_ptr&& p)	{ swap (p); return *this; }
    inline constexpr auto&	operator* (void) const		{ return *get(); }
    inline constexpr auto	operator-> (void) const		{ return get(); }
    inline constexpr auto	operator<=> (const unique_ptr& p) const = default;
    inline constexpr auto	operator<=> (const pointer p) const	{ return _p <=> p; }
private:
    pointer			_p;
};

template <typename T, typename... Args>
inline constexpr auto make_unique (Args&&... args) { return unique_ptr<T> (new T (forward<Args>(args)...)); }

//}}}-------------------------------------------------------------------
//{{{ rep_movs

inline constexpr auto rep_movs (const void*& f, size_t n, void*& r) {
    __builtin_memmove (r, f, n);
    advance (r, n);
    advance (f, n);
    return r;
}
template <typename T>
inline constexpr auto rep_movs (const T*& f, size_t n, T*& r) {
    __builtin_memmove (r, f, n*sizeof(T));
    advance (r, n);
    advance (f, n);
    return r;
}
#if __x86__
#define CWICLO_REP_MOVS(T,instr)	\
template <> inline constexpr auto rep_movs (const T*& f, size_t n, T*& r) {\
    if (!is_constant_evaluated())	\
	__asm__ volatile (instr:"+S"(f),"+D"(r),"+c"(n)::"cc","memory");\
    else {				\
	__builtin_memmove (r, f, n*sizeof(T));\
	advance (r, n);			\
	advance (f, n);			\
    }					\
    return r;				\
}
#if __x86_64__
CWICLO_REP_MOVS (uint64_t, "rep movsq")
CWICLO_REP_MOVS ( int64_t, "rep movsq")
#endif
CWICLO_REP_MOVS (uint32_t, "rep movsl")
CWICLO_REP_MOVS ( int32_t, "rep movsl")
CWICLO_REP_MOVS (uint16_t, "rep movsw")
CWICLO_REP_MOVS ( int16_t, "rep movsw")
CWICLO_REP_MOVS ( uint8_t, "rep movsb")
CWICLO_REP_MOVS (  int8_t, "rep movsb")
#undef CWICLO_REP_MOVS
#endif

//}}}-------------------------------------------------------------------
//{{{ rep_movs_rev

inline constexpr auto rep_movs_rev (const void*& f, size_t n, void*& r)
    { return __builtin_memmove (pointer_cast<char>(r)-n, pointer_cast<char>(f)-n, n); }
template <typename T>
inline constexpr auto rep_movs_rev (const T*& f, size_t n, T*& r)
    { return pointer_cast<T>(__builtin_memmove (r -= n, f -= n, n*sizeof(T))); }
#if __x86__
#define CWICLO_REP_MOVS(T,instr)\
template <> inline constexpr auto rep_movs_rev (const T*& f, size_t n, T*& r) {\
    if (!is_constant_evaluated()) {\
	__asm__ volatile ("std\n\t" instr "\n\tcld":"+S"(f),"+D"(r),"+c"(n)::"cc","memory");\
	return r;\
    } else\
	return pointer_cast<T>(__builtin_memmove (r -= n, f -= n, n*sizeof(T)));\
}
#if __x86_64__
CWICLO_REP_MOVS (uint64_t, "rep movsq")
CWICLO_REP_MOVS ( int64_t, "rep movsq")
#endif
CWICLO_REP_MOVS (uint32_t, "rep movsl")
CWICLO_REP_MOVS ( int32_t, "rep movsl")
CWICLO_REP_MOVS (uint16_t, "rep movsw")
CWICLO_REP_MOVS ( int16_t, "rep movsw")
CWICLO_REP_MOVS ( uint8_t, "rep movsb")
CWICLO_REP_MOVS (  int8_t, "rep movsb")
#undef CWICLO_REP_MOVS
#endif

//}}}-------------------------------------------------------------------
//{{{ rep_stos

template <typename T>
constexpr auto rep_stos (T* f, size_t n, const T& v) {
    for (auto l = next(f,n); f < l; advance(f))
	*f = v;
    return f;
}
#if __x86__
#define CWICLO_REP_STOS(T,instr)\
template <> inline constexpr auto rep_stos (T* f, size_t n, const T& v) {\
    if (!is_constant_evaluated())\
	__asm__ volatile (instr:"+D"(f),"+c"(n):"a"(v):"cc","memory");\
    else for (auto l = next(f,n); f < l; advance(f))\
	*f = v;\
    return f;\
}
#if __x86_64__
CWICLO_REP_STOS (uint64_t, "rep stosq")
CWICLO_REP_STOS ( int64_t, "rep stosq")
#endif
CWICLO_REP_STOS (uint32_t, "rep stosl")
CWICLO_REP_STOS ( int32_t, "rep stosl")
CWICLO_REP_STOS (uint16_t, "rep stosw")
CWICLO_REP_STOS ( int16_t, "rep stosw")
CWICLO_REP_STOS ( uint8_t, "rep stosb")
CWICLO_REP_STOS (  int8_t, "rep stosb")
#undef CWICLO_REP_STOS
#endif

//}}}-------------------------------------------------------------------
//{{{ copy

template <typename II, typename OI>
constexpr auto copy_n (II f, size_t n, OI r)
{
    #if __x86__
    using ivalue_type = make_unsigned_t<remove_const_t<typename iterator_traits<II>::value_type>>;
    using ovalue_type = make_unsigned_t<remove_const_t<typename iterator_traits<OI>::value_type>>;
    if constexpr (is_pointer<II>::value && is_pointer<OI>::value && is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value) {
	#define CWICLO_CALL_REP_MOVS(type) do {\
	    auto ef = pointer_cast<const type>(to_address(f));\
	    auto er = pointer_cast<type>(to_address(r));\
	    r = OI (rep_movs (ef, n*sizeof(ovalue_type)/sizeof(type), er)); } while (false)
	#if __x86_64__
	if constexpr (!(sizeof(ovalue_type)%8))
	    CWICLO_CALL_REP_MOVS (uint64_t);
	else
	#endif
	if constexpr (!(sizeof(ovalue_type)%4))
	    CWICLO_CALL_REP_MOVS (uint32_t);
	else if constexpr (!(sizeof(ovalue_type)%2))
	    CWICLO_CALL_REP_MOVS (uint16_t);
	else
	    CWICLO_CALL_REP_MOVS (uint8_t);
	#undef CWICLO_CALL_REP_MOVS
    } else
    #endif
    for (auto l = next(f,n); f < l; advance(r), advance(f))
	*r = *f;
    return r;
}

template <typename II, typename OI>
constexpr auto copy (II f, II l, OI r)
{
    using ivalue_type = make_unsigned_t<remove_const_t<typename iterator_traits<II>::value_type>>;
    using ovalue_type = make_unsigned_t<remove_const_t<typename iterator_traits<OI>::value_type>>;
    if constexpr (is_pointer<II>::value && is_pointer<OI>::value && is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return copy_n (f, distance(f,l), r);
    for (; f < l; advance(r), advance(f))
	*r = *f;
    return r;
}

template <typename C, typename OI>
inline constexpr auto copy (const C& c, OI r)
    { return copy_n (begin(c), size(c), r); }

template <typename II, typename OI>
constexpr auto copy_backward_n (II f, size_t n, OI r)
{
    #if __x86__
    using ivalue_type = make_unsigned_t<remove_const_t<typename iterator_traits<II>::value_type>>;
    using ovalue_type = make_unsigned_t<remove_const_t<typename iterator_traits<OI>::value_type>>;
    if constexpr (is_pointer<II>::value && is_pointer<OI>::value && is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value) {
	#define CWICLO_CALL_REP_MOVS(type) do {\
	    auto ef = pointer_cast<const type>(to_address(next(f,n)))-1;\
	    auto er = pointer_cast<type>(to_address(next(r,n)))-1;\
	    rep_movs_rev (ef, n*sizeof(ovalue_type)/sizeof(type), er); } while (false)
	#if __x86_64__
	if constexpr (!(sizeof(ovalue_type)%8))
	    CWICLO_CALL_REP_MOVS (uint64_t);
	else
	#endif
	if constexpr (!(sizeof(ovalue_type)%4))
	    CWICLO_CALL_REP_MOVS (uint32_t);
	else if constexpr (!(sizeof(ovalue_type)%2))
	    CWICLO_CALL_REP_MOVS (uint16_t);
	else
	    CWICLO_CALL_REP_MOVS (uint8_t);
	#undef CWICLO_CALL_REP_MOVS
    } else
    #endif // !__x86__
    while (n--)
	r[n] = f[n];
    return r;
}

template <typename II, typename OI>
constexpr auto copy_backward (II f, II l, OI r)
{
    using ivalue_type = make_unsigned_t<remove_const_t<typename iterator_traits<II>::value_type>>;
    using ovalue_type = make_unsigned_t<remove_const_t<typename iterator_traits<OI>::value_type>>;
    if constexpr (is_pointer<II>::value && is_pointer<OI>::value && is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return copy_backward_n (f, distance(f,l), r);
    while (f < l)
	*--r = *--l;
    return r;
}

template <typename C, typename OI>
inline constexpr auto copy_backward (const C& c, OI r)
    { return copy_backward_n (begin(c), size(c), r); }

//}}}-------------------------------------------------------------------
//{{{ fill

template <typename I, typename T>
constexpr auto fill_n (I f, size_t n, const T& v)
{
    using ivalue_type = make_unsigned_t<remove_const_t<T>>;
    using ovalue_type = make_unsigned_t<remove_const_t<typename iterator_traits<I>::value_type>>;
    constexpr bool canstos = is_pointer<I>::value && is_trivially_assignable<ovalue_type>::value && is_same<ivalue_type,ovalue_type>::value;
    if constexpr (canstos && sizeof(ovalue_type) == 1)
	{ __builtin_memset (to_address(f), bit_cast<uint8_t>(v), n); advance(f,n); }
#define CWICLO_IF_CALL_REP_STOS(type)\
    else if constexpr (canstos && sizeof(ovalue_type) == sizeof(type))\
	f = I (rep_stos (pointer_cast<type>(to_address(f)), n, bit_cast<type>(v)))
    CWICLO_IF_CALL_REP_STOS (uint16_t);
    CWICLO_IF_CALL_REP_STOS (uint32_t);
    CWICLO_IF_CALL_REP_STOS (uint64_t);
#undef CWICLO_IF_CALL_REP_STOS
    else for (auto l = next(f,n); f < l; advance(f))
	*f = v;
    return f;
}

template <typename I, typename T>
constexpr auto fill (I f, I l, const T& v)
{
    using ivalue_type = make_unsigned_t<remove_const_t<T>>;
    using ovalue_type = make_unsigned_t<remove_const_t<typename iterator_traits<I>::value_type>>;
    if constexpr (is_pointer<I>::value && is_trivially_assignable<ovalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return fill_n (f, distance(f,l), v);
    for (; f < l; advance(f))
	*f = v;
    return f;
}

template <typename C, typename T>
inline constexpr auto fill (C& c, const T& v)
    { return fill_n (begin(c), size(c), v); }

template <typename I>
inline constexpr auto zero_fill_n (I f, size_t n)
{
    using ovalue_type = make_unsigned_t<remove_const_t<typename iterator_traits<I>::value_type>>;
    if constexpr (is_pointer<I>::value && is_trivially_assignable<ovalue_type>::value)
	{ __builtin_memset (to_address(f), 0, n*sizeof(ovalue_type)); advance(f,n); }
    else for (auto l = next(f,n); f < l; advance(f))
	*f = ovalue_type{};
    return f;
}

template <typename I>
inline constexpr auto zero_fill (I f, I l)
{
    using ovalue_type = make_unsigned_t<remove_const_t<typename iterator_traits<I>::value_type>>;
    if constexpr (is_pointer<I>::value && is_trivially_assignable<ovalue_type>::value)
	f = zero_fill_n (f, distance(f,l));
    else for (; f < l; advance(f))
	*f = ovalue_type{};
    return f;
}

template <typename C>
inline constexpr auto zero_fill (C& c)
    { return zero_fill_n (begin(c), size(c)); }

//}}}-------------------------------------------------------------------
//{{{ shift and rotate

template <typename I>
constexpr auto shift_left (I f, I l, size_t n)
{
    auto m = next(f,n);
    assert (m >= f && m <= l);
    return copy_n (m, distance(m,l), f);
}

template <typename C, typename T>
inline constexpr auto shift_left (C& c, size_t n)
    { return shift_left (begin(c), end(c), n); }

template <typename I>
constexpr auto shift_right (I f, I l, size_t n)
{
    auto m = next(f,n);
    assert (m >= f && m <= l);
    return copy_backward_n (f, distance(m,l), m);
}

template <typename C, typename T>
inline constexpr auto shift_right (C& c, size_t n)
    { return shift_right (begin(c), end(c), n); }

extern "C" void brotate (void* vf, void* vm, void* vl);

template <typename T>
T* rotate (T* f, T* m, T* l)
    { brotate (f, m, l); return f; }

//}}}-------------------------------------------------------------------
//{{{ construct and destroy

/// Calls the placement new on \p p.
template <typename T>
inline constexpr auto construct_at (T* p)
    { return new (p) T; }

/// Calls the placement new on \p p.
template <typename T>
inline constexpr auto construct_at (T* p, const T& value)
    { return new (p) T (value); }

/// Calls the placement new on \p p with \p args.
template <typename T, typename... Args>
inline constexpr auto construct_at (T* p, Args&&... args)
    { return new (p) T (forward<Args>(args)...); }

/// Calls the placement new on \p p.
template <typename I>
constexpr auto uninitialized_default_construct (I f, I l)
{
    if constexpr (is_trivially_constructible<typename iterator_traits<I>::value_type>::value) {
	if (f < l)
	    zero_fill (f, l);
    } else for (; f < l; advance(f))
	construct_at (f);
    return f;
}

/// Calls the placement new on \p p.
template <typename I>
constexpr auto uninitialized_default_construct_n (I f, size_t n)
    { return uninitialized_default_construct (f, next(f,n)); }

/// Calls the destructor of \p p without calling delete.
template <typename T>
inline constexpr void destroy_at (T* p)
    { p->~T(); }

/// Calls the destructor on elements in range [f, l) without calling delete.
template <typename I>
constexpr void destroy (I f [[maybe_unused]], I l [[maybe_unused]])
{
    if constexpr (!is_trivially_destructible<typename iterator_traits<I>::value_type>::value) {
	for (; f < l; advance(f))
	    destroy_at (f);
    }
#ifndef NDEBUG
    else if (f < l)
	__builtin_memset (static_cast<void*>(to_address(f)), '\xcd', distance(f,l)*sizeof(*to_address(f)));
#endif
}

/// Calls the destructor on elements in range [f, f+n) without calling delete.
template <typename I>
inline constexpr void destroy_n (I f, size_t n)
    { return destroy (f, next(f,n)); }

//}}}-------------------------------------------------------------------
//{{{ uninitialized fill and copy

/// Copies [f, l) into r by calling copy constructors in r.
template <typename II, typename OI>
constexpr auto uninitialized_copy (II f, II l, OI r)
{
    using ivalue_type = remove_const_t<typename iterator_traits<II>::value_type>;
    using ovalue_type = remove_const_t<typename iterator_traits<OI>::value_type>;
    if constexpr (is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return copy (f, l, r);
    for (; f < l; advance(r), advance(f))
	construct_at (to_address(r), *f);
    return r;
}

/// Copies [f, f + n) into r by calling copy constructors in r.
template <typename II, typename OI>
constexpr auto uninitialized_copy_n (II f, size_t n, OI r)
{
    using ivalue_type = remove_const_t<typename iterator_traits<II>::value_type>;
    using ovalue_type = remove_const_t<typename iterator_traits<OI>::value_type>;
    if constexpr (is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return copy_n (f, n, r);
    for (auto i = 0u; i < n; ++i)
	construct_at (&r[i], f[i]);
    return r;
}

/// Copies [f, l) into r by calling move constructors in r.
template <typename II, typename OI>
constexpr auto uninitialized_move (II f, II l, OI r)
{
    using ivalue_type = remove_const_t<typename iterator_traits<II>::value_type>;
    using ovalue_type = remove_const_t<typename iterator_traits<OI>::value_type>;
    if constexpr (is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return copy (f, l, r);
    for (; f < l; advance(r), advance(f))
	construct_at (to_address(r), move(*f));
    return r;
}

/// Copies [f, f + n) into r by calling move constructors in r.
template <typename II, typename OI>
constexpr auto uninitialized_move_n (II f, size_t n, OI r)
{
    using ivalue_type = remove_const_t<typename iterator_traits<II>::value_type>;
    using ovalue_type = remove_const_t<typename iterator_traits<OI>::value_type>;
    if constexpr (is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	return copy_n (f, n, r);
    for (auto i = 0u; i < n; ++i)
	construct_at (&r[i], move(f[i]));
    return r;
}

/// Calls construct on all elements in [f, l) with value \p v.
template <typename I, typename T>
constexpr void uninitialized_fill (I f, I l, const T& v)
{
    using ivalue_type = remove_const_t<T>;
    using ovalue_type = remove_const_t<typename iterator_traits<I>::value_type>;
    if constexpr (is_trivially_constructible<ovalue_type>::value && is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	fill (f, l, v);
    else for (; f < l; advance(f))
	construct_at (to_address(f), v);
}

/// Calls construct on all elements in [f, f + n) with value \p v.
template <typename I, typename T>
constexpr auto uninitialized_fill_n (I f, size_t n, const T& v)
{
    using ivalue_type = remove_const_t<T>;
    using ovalue_type = remove_const_t<typename iterator_traits<I>::value_type>;
    if constexpr (is_trivially_constructible<ovalue_type>::value && is_trivially_copyable<ivalue_type>::value && is_same<ivalue_type,ovalue_type>::value)
	fill_n (f, n, v);
    else for (auto i = 0u; i < n; ++i)
	construct_at (&f[i], v);
    return f;
}

//}}}-------------------------------------------------------------------
//{{{ Additional C++ ABI support

extern "C" void print_backtrace (void);
#ifndef UC_VERSION
extern "C" void hexdump (const void* vp, size_t n);
#endif

template <typename C>
inline void hexdump (const C& c)
    { auto p = to_address(begin(c)); hexdump (p, sizeof(*p)*size(c)); }

} // namespace cwiclo

namespace std {

[[noreturn]] void terminate (void) noexcept;

} // namespace std
namespace __cxxabiv1 {

using __guard = cwiclo::atomic_flag;

extern "C" {

[[noreturn]] void __cxa_call_unexpected (void) noexcept;
[[noreturn]] void __cxa_pure_virtual (void) noexcept;
[[noreturn]] void __cxa_deleted_virtual (void) noexcept;
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
