// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "config.h"

//{{{ Type modifications -----------------------------------------------
namespace cwiclo {

/// true or false templatized constant for metaprogramming
template <typename T, T v>
struct integral_constant {
    using value_type = T;
    using type = integral_constant<value_type,v>;
    static constexpr const T value = v;
    constexpr operator value_type() const { return value; }
    constexpr auto operator()() const { return value; }
};

using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;

template <typename T, typename U> struct is_same : public false_type {};
template <typename T> struct is_same<T,T> : public true_type {};

template <typename T> struct type_identity		{ using type = T; };
template <typename T> using type_identity_t = typename type_identity<T>::type;

template <typename T> struct remove_reference		{ using type = T; };
template <typename T> struct remove_reference<T&>	{ using type = T; };
template <typename T> struct remove_reference<T&&>	{ using type = T; };
template <typename T> using remove_reference_t = typename remove_reference<T>::type;

template <typename T> struct remove_const		{ using type = T; };
template <typename T> struct remove_const<const T>	{ using type = T; };
template <typename T> struct remove_const<const T*>	{ using type = T*; };
template <typename T> struct remove_const<const T&>	{ using type = T&; };
template <typename T> using remove_const_t = typename remove_const<T>::type;

template <typename T> struct add_const		{ using type = const T; };
template <typename T> struct add_const<const T> { using type = const T; };
template <typename T> struct add_const<const T*>{ using type = const T*; };
template <typename T> struct add_const<const T&>{ using type = const T&; };
template <typename T> struct add_const<T*>	{ using type = const T*; };
template <typename T> struct add_const<T&>	{ using type = const T&; };
template <typename T> using add_const_t = typename add_const<T>::type;

template <typename T> T&& declval (void);

template <typename T> struct is_trivial : public integral_constant<bool, __is_trivial(T)> {};
template <typename T> struct is_trivially_constructible : public integral_constant<bool, __has_trivial_constructor(T)> {};
template <typename T> struct is_trivially_destructible : public integral_constant<bool, __has_trivial_destructor(T)> {};
template <typename T> struct is_trivially_copyable : public integral_constant<bool, __has_trivial_copy(T)> {};
template <typename T> struct is_trivially_assignable : public integral_constant<bool, __has_trivial_assign(T)> {};

template <typename T> struct make_unsigned	{ using type = T; };
template <> struct make_unsigned<char>		{ using type = unsigned char; };
template <> struct make_unsigned<short>		{ using type = unsigned short; };
template <> struct make_unsigned<int>		{ using type = unsigned int; };
template <> struct make_unsigned<long>		{ using type = unsigned long; };
template <typename T> using make_unsigned_t = typename make_unsigned<T>::type;

template <typename T> struct is_signed : public integral_constant<bool, !is_same<T,make_unsigned_t<T>>::value> {};

template <typename T> struct bits_in_type	{ static constexpr const size_t value = sizeof(T)*8; };

template <typename T> constexpr add_const_t<T>& as_const (T& t) { return t; }
template <typename T> void as_const (T&&) = delete;

template <typename T, typename F> constexpr decltype(auto) bit_cast (F& v)
    { union FTU { F f; T t; }; return reinterpret_cast<FTU&>(v).t; }
template <typename T, typename F> constexpr decltype(auto) bit_cast (const F& v)
    { union FTU { F f; T t; }; return reinterpret_cast<const FTU&>(v).t; }
template <typename T, typename F> constexpr void bit_cast (F&& v) = delete;

// constexpr cast between pointers without reinterpret_cast
template <typename T, typename F> constexpr T* pointer_cast (F* p)
    { return static_cast<T*>(static_cast<void*>(p)); }
template <typename T, typename F> constexpr const T* pointer_cast (const F* p)
    { return static_cast<const T*>(static_cast<const void*>(p)); }

// constexpr conversion from pointer to uintptr_t
template <typename T> constexpr uintptr_t pointer_value (T p)
    { return pointer_cast<char>(p)-static_cast<const char*>(nullptr); }

// Create a passthrough non-const member function from a call to a const member function
#define UNCONST_MEMBER_FN(f,...)	\
    const_cast<remove_const_t<decltype((const_cast<add_const_t<decltype(this)>>(this)->f(__VA_ARGS__)))>>(const_cast<add_const_t<decltype(this)>>(this)->f(__VA_ARGS__));

// Defines a has_member_function_name template where has_member_function_name<O>::value is true when O::name exists
// Example: HAS_MEMBER_FUNCTION(stream_read, read, void (O::*)(istream&)); has_member_function_read<vector<int>>::value == true
#define HAS_MEMBER_FUNCTION(tname, fname, signature)\
    template <typename FO>			\
    class __has_member_function_##tname {	\
	template <typename O> static true_type found (signature);\
	template <typename O> static auto found (decltype(&O::fname), void*) -> decltype(found(&O::fname));\
	template <typename O> static false_type found (...);\
    public:					\
	using type = decltype(found<FO>(nullptr,nullptr));\
    };						\
    template <typename FO>			\
    struct has_member_function_##tname : public __has_member_function_##tname<FO>::type {}

//}}}-------------------------------------------------------------------
//{{{ numeric limits

template <typename T>
class numeric_limits {
    using base_type = remove_reference_t<T>;
public:
    static constexpr const bool is_signed = ::cwiclo::is_signed<T>::value;	///< True if the type is signed.
    static constexpr const bool is_integral = is_trivial<T>::value;	///< True if fixed size and cast-copyable.
    static constexpr auto min (void) {
	if constexpr (is_signed)
	    return base_type(1)<<(bits_in_type<base_type>::value-1);
	else
	    return base_type(0);
    }
    static constexpr auto max (void)
	{ return base_type(make_unsigned_t<base_type>(min())-1); }
};

//}}}-------------------------------------------------------------------
//{{{ Array adapters

template <typename T> constexpr decltype(auto)	begin (T& c)		{ return c.begin(); }
template <typename T> constexpr decltype(auto)	begin (const T& c)	{ return c.begin(); }
template <typename T, size_t N> constexpr auto	begin (T (&a)[N])	{ return &a[0]; }
template <typename T> constexpr decltype(auto)	cbegin (const T& c)	{ return begin(c); }
template <typename T> constexpr decltype(auto)	end (T& c)		{ return c.end(); }
template <typename T> constexpr decltype(auto)	end (const T& c)	{ return c.end(); }
template <typename T, size_t N> constexpr auto	end (T (&c)[N])		{ return &c[N]; }
template <typename T> constexpr decltype(auto)	cend (const T& c)	{ return end(c); }
template <typename T> constexpr auto		size (const T& c)	{ return c.size(); }
template <typename T, size_t N> constexpr auto	size (const T (&)[N])	{ return N; }
template <typename T> [[nodiscard]] constexpr bool empty (const T& c)	{ return !c.size(); }
template <typename T> constexpr decltype(auto)	data (T& c)		{ return c.data(); }
template <typename T> constexpr decltype(auto)	data (const T& c)	{ return c.data(); }
template <typename T, size_t N> constexpr auto	data(T (&c)[N])		{ return &c[0]; }

/// Expands into a ptr,size expression for the given static vector; useful as link arguments.
#define ARRAY_BLOCK(v)	::cwiclo::data(v), ::cwiclo::size(v)
/// Expands into a begin,end expression for the given static vector; useful for algorithm arguments.
#define ARRAY_RANGE(v)	::cwiclo::begin(v), ::cwiclo::end(v)
/// Expands into a ptr,sizeof expression for the given static data block
#define DATA_BLOCK(v)	::cwiclo::data(v), sizeof(v)

/// Shorthand for container iteration.
#define foreach(i,ctr)	for (auto i = ::cwiclo::begin(ctr); i < ::cwiclo::end(ctr); ++i)
/// Shorthand for container reverse iteration.
#define eachfor(i,ctr)	for (auto i = ::cwiclo::end(ctr); i-- > ::cwiclo::begin(ctr);)

//}}}----------------------------------------------------------------------
//{{{ bswap

[[nodiscard]] constexpr uint8_t bswap (uint8_t v)	{ return v; }
[[nodiscard]] constexpr uint16_t bswap (uint16_t v)	{ return __builtin_bswap16 (v); }
[[nodiscard]] constexpr uint32_t bswap (uint32_t v)	{ return __builtin_bswap32 (v); }
[[nodiscard]] constexpr uint64_t bswap (uint64_t v)	{ return __builtin_bswap64 (v); }
[[nodiscard]] constexpr int8_t bswap (int8_t v)		{ return v; }
[[nodiscard]] constexpr int16_t bswap (int16_t v)	{ return __builtin_bswap16 (v); }
[[nodiscard]] constexpr int32_t bswap (int32_t v)	{ return __builtin_bswap32 (v); }
[[nodiscard]] constexpr int64_t bswap (int64_t v)	{ return __builtin_bswap64 (v); }

enum class endian {
    little	= __ORDER_LITTLE_ENDIAN__,
    big		= __ORDER_BIG_ENDIAN__,
    native	= __BYTE_ORDER__
};

template <typename T> [[nodiscard]] constexpr T le_to_native (const T& v) {
    if constexpr (endian::native == endian::big)
	return bswap(v);
    else
	return v;
}
template <typename T> [[nodiscard]] constexpr T be_to_native (const T& v) {
    if constexpr (endian::native == endian::little)
	return bswap(v);
    else
	return v;
}
template <typename T> [[nodiscard]] constexpr T native_to_le (const T& v) { return le_to_native(v); }
template <typename T> [[nodiscard]] constexpr T native_to_be (const T& v) { return be_to_native(v); }

//}}}----------------------------------------------------------------------
//{{{ min and max

template <typename T>
constexpr const T& min (const T& a, const type_identity_t<T>& b)
    { return a < b ? a : b; }
template <typename T>
constexpr const T& max (const T& a, const type_identity_t<T>& b)
    { return b < a ? a : b; }

//}}}----------------------------------------------------------------------
//{{{ sign and absv

template <typename T>
constexpr bool is_negative (const T& v) {
    if constexpr (is_signed<T>::value)
	return v < 0;
    return false;
}

template <typename T>
constexpr auto sign (T v)
    { return (0 < v) - is_negative(v); }
template <typename T>
constexpr make_unsigned_t<T> absv (T v)
    { return is_negative(v) ? -v : v; }
template <typename T>
[[nodiscard]] constexpr T copy_sign (T f, type_identity_t<T> t)
    { return is_negative<T>(f) ? -t : t; }

//}}}----------------------------------------------------------------------
//{{{ ceilg and other alignment functionality

template <typename T>
[[nodiscard]] constexpr T floorg (T n, type_identity_t<T> g)
    { return n - n % g; }
template <typename T>
[[nodiscard]] constexpr auto ceilg (T n, type_identity_t<T> g)
    { return floorg<T>(n + copy_sign<T>(n, g-1), g); }
template <typename T>
[[nodiscard]] constexpr auto roundg (T n, type_identity_t<T> g)
    { return floorg<T>(n + copy_sign<T>(n, g/2), g); }

// Pointer alignment requires additional tricks because % is not allowed
template <typename T>
[[nodiscard]] constexpr auto floorg (T* p, uintptr_t g)
    { return p - pointer_value(p) % g; }
template <typename T>
[[nodiscard]] constexpr auto floorg (const T* p, uintptr_t g)
    { return p - pointer_value(p) % g; }
template <typename T>
[[nodiscard]] constexpr auto ceilg (T* p, type_identity_t<T> g)
    { return floorg<T>(p + g-1, g); }
template <typename T>
[[nodiscard]] constexpr auto ceilg (const T* p, type_identity_t<T> g)
    { return floorg<T>(p + g-1, g); }

template <typename T>
[[nodiscard]] constexpr auto assume_aligned (T* p, size_t g, size_t o = 0)
    { return static_cast<T*>(__builtin_assume_aligned (p,g,o)); }
template <typename T>
[[nodiscard]] constexpr auto assume_aligned (const T* p, size_t g, size_t o = 0)
    { return static_cast<const T*>(__builtin_assume_aligned (p,g,o)); }

template <typename T>
[[nodiscard]] constexpr T divide_ceil (T n1, type_identity_t<T> n2)
    { return (n1 + copy_sign<T>(n1,n2-1)) / n2; }
template <typename T>
[[nodiscard]] constexpr T divide_round (T n1, type_identity_t<T> n2)
    { return (n1 + copy_sign<T>(n1,n2/2)) / n2; }

template <typename T>
constexpr bool divisible_by (T n, type_identity_t<T> g)
    { return !(n % g); }
template <typename T>
constexpr bool divisible_by (const T* p, type_identity_t<T> g)
    { return !(pointer_value(p) % g); }

template <typename T>
constexpr T midpoint (T a, T b)
    { auto d = b-a; return a + make_unsigned_t<decltype(d)>(d)/2; }

template <typename T>
[[nodiscard]] constexpr make_unsigned_t<T> square (T n)
    { return n*n; }

//}}}----------------------------------------------------------------------
//{{{ Bit manipulation

template <typename T>
constexpr bool get_bit (T v, unsigned i)
    { return (v>>i)&1; }
template <typename T = unsigned>
constexpr T bit_mask (unsigned i)
    { return T(1)<<i; }
template <typename T>
constexpr void set_bit (T& v, unsigned i, bool b=true)
    { auto m = bit_mask<T>(i); v=b?(v|m):(v&~m); }
template <typename T>
[[nodiscard]] constexpr auto bit_rol (T v, type_identity_t<T> n)
    { return (v << n) | (v >> (bits_in_type<T>::value-n)); }
template <typename T>
[[nodiscard]] constexpr auto bit_ror (T v, type_identity_t<T> n)
    { return (v >> n) | (v << (bits_in_type<T>::value-n)); }

template <typename T>
constexpr unsigned log2p1 (T v)
{
    if constexpr (sizeof(T) <= sizeof(int))
	return bits_in_type<int>::value-__builtin_clz(v);
    else
	return bits_in_type<long>::value-__builtin_clzl(v);
}

template <typename T>
constexpr T ceil2 (T v)
    { return T(1)<<log2p1(v-1); }

template <typename T>
constexpr bool ispow2 (T v)
    { return !(v&(v-1)); }

//}}}----------------------------------------------------------------------
//{{{ atomic_flag

enum class memory_order : int {
    relaxed = __ATOMIC_RELAXED,
    consume = __ATOMIC_CONSUME,
    acquire = __ATOMIC_ACQUIRE,
    release = __ATOMIC_RELEASE,
    acq_rel = __ATOMIC_ACQ_REL,
    seq_cst = __ATOMIC_SEQ_CST
};

namespace {

// Use in lock wait loops to relax the CPU load
inline static void tight_loop_pause (void)
{
    #if __x86__
	__builtin_ia32_pause();
    #else
	usleep (1);
    #endif
}

template <typename T>
[[nodiscard]] static constexpr T kill_dependency (T v)
    { return T(v); }
inline static void atomic_thread_fence (memory_order order)
    { __atomic_thread_fence (int(order)); }
inline static void atomic_signal_fence (memory_order order)
    { __atomic_signal_fence (int(order)); }

} // namespace

class atomic_flag {
    bool		_v = false;
public:
    constexpr		atomic_flag (void) = default;
    constexpr		atomic_flag (bool v)	: _v(v) {}
    constexpr		atomic_flag (const atomic_flag&) = delete;
    void		operator= (const atomic_flag&) = delete;
    inline void		clear (memory_order order = memory_order::release)
			    { __atomic_clear (&_v, int(order)); }
    inline bool		test_and_set (memory_order order = memory_order::acq_rel)
			    { return __atomic_test_and_set (&_v, int(order)); }
};
#define ATOMIC_FLAG_INIT	{false}

class atomic_scope_lock {
    atomic_flag& _f;
public:
    explicit atomic_scope_lock (atomic_flag& f) : _f(f) { while (_f.test_and_set()) tight_loop_pause(); }
    ~atomic_scope_lock (void) { _f.clear(); }
};

//}}}-------------------------------------------------------------------
//{{{ SIMD intrinsic types

// Intrinsic functions are typed for a specific vector type, such as
// float[4]. In cwiclo, pretty much all of the uses for SSE do bulk
// memory moves, for which strict typing is inconvenient and usually
// wrong anyway. This union is used to present appropriate type to
// intrinsic functions.

union alignas(16) simd16_t {
    using ui_t	= uint32_t	__attribute__((vector_size(16)));
    using si_t	= int32_t	__attribute__((vector_size(16)));
    using uq_t	= uint64_t	__attribute__((vector_size(16)));
    using sq_t	= int64_t	__attribute__((vector_size(16)));
    using sf_t	= float		__attribute__((vector_size(16)));
    using sd_t	= double	__attribute__((vector_size(16)));

    sf_t	sf;
    sd_t	sd;
    ui_t	ui;
    si_t	si;
    uq_t	uq;
    sq_t	sq;
    uint8_t	aub [16];
    int8_t	asb [16];
    uint16_t	auw [8];
    int16_t	asw [8];
    uint32_t	aui [4];
    int32_t	asi [4];
    uint64_t	auq [2];
    int64_t	asq [2];
    float	asf [4];
    double	asd [2];

    [[nodiscard]] CONST static constexpr auto zero (void)
	{ return simd16_t {0,0,0,0}; }
};

} // namespace cwiclo
//}}}-------------------------------------------------------------------
