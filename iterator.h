// This file is part of the cwiclo project
//
// Copyright (c) 2020 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "utility.h"

//{{{ iterator_traits --------------------------------------------------
namespace cwiclo {

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
//{{{ Iterator-pointer utilities

template <typename T>
constexpr T* to_address (T* p)
    { return p; }
template <typename P>
constexpr auto to_address (const P& p)
    { return to_address (p.operator->()); }

template <typename I>
constexpr I next (I f, size_t n = 1)
    { return f+n; }
template <> constexpr void* next (void* f, size_t n)
    { return next (static_cast<char*>(f), n); }
template <> constexpr const void* next (const void* f, size_t n)
    { return next (static_cast<const char*>(f), n); }

template <typename I>
constexpr I prev (I f, size_t n = 1)
    { return f-n; }
template <> constexpr void* prev (void* f, size_t n)
    { return prev (static_cast<char*>(f), n); }
template <> constexpr const void* prev (const void* f, size_t n)
    { return prev (static_cast<const char*>(f), n); }

template <typename I>
constexpr auto distance (I f, I l)
    { return l-f; }
template <> constexpr auto distance (void* f, void* l)
    { return distance (static_cast<char*>(f), static_cast<char*>(l)); }
template <> constexpr auto distance (const void* f, const void* l)
    { return distance (static_cast<const char*>(f), static_cast<const char*>(l)); }

template <typename T>
constexpr void advance (T*& f, size_t n = 1) { f = next(f,n); }
template <typename I> constexpr void advance (I& f) { ++f; }
template <typename I> constexpr void advance (I& f, size_t n) { f += n; }

template <typename I>
inline constexpr void iter_swap (I a, I b)
    { swap (*a, *b); }

//}}}-------------------------------------------------------------------
//{{{ reverse_iterator

template <typename I>
class reverse_iterator {
public:
    using value_type		= typename iterator_traits<I>::value_type;
    using difference_type	= typename iterator_traits<I>::difference_type;
    using pointer		= typename iterator_traits<I>::pointer;
    using reference		= typename iterator_traits<I>::reference;
public:
    constexpr		reverse_iterator (void)		: _i() {}
    constexpr explicit	reverse_iterator (const I& i)	: _i (i) {}
    constexpr auto&	base (void) const		{ return _i; }
    constexpr auto&	operator* (void) const		{ auto r (_i); return *--r; }
    constexpr auto	operator-> (void) const		{ auto r (_i); return to_address(--r); }
    constexpr auto&	operator[] (size_t n) const	{ return *(*this + n); }
    constexpr auto&	operator++ (void)		{ --_i; return *this; }
    constexpr auto&	operator-- (void)		{ ++_i; return *this; }
    constexpr auto	operator++ (int)		{ auto r (*this); operator++(); return r; }
    constexpr auto	operator-- (int)		{ auto r (*this); operator--(); return r; }
    constexpr auto&	operator+= (size_t n)		{ _i -= n; return *this; }
    constexpr auto&	operator-= (size_t n)		{ _i += n; return *this; }
    constexpr auto	operator+ (size_t n) const	{ return reverse_iterator (_i-n); }
    constexpr auto	operator- (size_t n) const	{ return reverse_iterator (_i+n); }
    constexpr auto	operator- (const reverse_iterator& i) const	{ return distance (i._i, _i); }
    constexpr bool	operator< (const reverse_iterator& i) const	{ return i._i < _i; }
    constexpr bool	operator== (const reverse_iterator& i) const	{ return _i == i._i; }
private:
    I			_i;
};

template <typename I>
inline constexpr auto make_reverse_iterator (const I& i)
    { return reverse_iterator<I>(i); }

//}}}-------------------------------------------------------------------
//{{{ move_iterator

template <typename I>
class move_iterator {
public:
    using value_type		= typename iterator_traits<I>::value_type;
    using difference_type	= typename iterator_traits<I>::difference_type;
    using pointer		= typename iterator_traits<I>::pointer;
    using reference		= value_type&&;
public:
    constexpr		move_iterator (void)		: _i() {}
    constexpr explicit	move_iterator (const I& i)	: _i (i) {}
    constexpr auto&	base (void) const		{ return _i; }
    constexpr reference	operator* (void) const		{ return move(*_i); }
    constexpr auto	operator-> (void) const		{ return to_address(_i); }
    constexpr reference	operator[] (size_t n) const	{ return move(*(*this + n)); }
    constexpr auto&	operator++ (void)		{ ++_i; return *this; }
    constexpr auto&	operator-- (void)		{ --_i; return *this; }
    constexpr auto	operator++ (int)		{ auto r (*this); operator++(); return r; }
    constexpr auto	operator-- (int)		{ auto r (*this); operator--(); return r; }
    constexpr auto&	operator+= (size_t n)		{ _i += n; return *this; }
    constexpr auto&	operator-= (size_t n)		{ _i -= n; return *this; }
    constexpr auto	operator+ (size_t n) const	{ return move_iterator (_i-n); }
    constexpr auto	operator- (size_t n) const	{ return move_iterator (_i+n); }
    constexpr bool	operator< (const I& i) const	{ return _i < i; }
    constexpr bool	operator== (const I& i) const	{ return _i == i; }
    constexpr auto	operator- (const move_iterator& i) const	{ return distance (_i, i._i); }
    constexpr bool	operator< (const move_iterator& i) const	{ return _i < i._i; }
    constexpr bool	operator== (const move_iterator& i) const	{ return _i == i._i; }
private:
    I			_i;
};

template <typename I>
inline constexpr move_iterator<I> make_move_iterator (const I& i)
    { return move_iterator<I>(i); }

//}}}-------------------------------------------------------------------
//{{{ insert_iterator

template <typename C>
class insert_iterator {
public:
    using iterator		= typename C::iterator;
    using value_type		= typename C::value_type;
    using difference_type	= typename C::difference_type;
    using pointer		= typename C::pointer;
    using reference		= typename C::reference;
    using const_reference	= const value_type&;
public:
    constexpr explicit	insert_iterator (C& c, iterator ip) : _ctr(c), _ip(ip) {}
    constexpr auto&	operator* (void)  { return *this; }
    constexpr auto&	operator++ (void) { ++_ip; return *this; }
    constexpr auto	operator++ (int)  { auto r (*this); operator++(); return r; }
    inline auto&	operator= (const_reference v)
			    { _ip = _ctr.insert (_ip, v); return *this; }
private:
    C&			_ctr;
    iterator		_ip;
};

template <typename C>
inline constexpr auto inserter (C& ctr, const typename C::iterator& ip)
    { return insert_iterator<C> (ctr, ip); }

//}}}-------------------------------------------------------------------
//{{{ back_insert_iterator

template <class C>
class back_insert_iterator {
public:
    using value_type		= typename C::value_type;
    using difference_type	= typename C::difference_type;
    using pointer		= typename C::pointer;
    using reference		= typename C::reference;
    using const_reference	= const value_type&;
public:
    constexpr explicit	back_insert_iterator (C& ctr) : _ctr (ctr) {}
    constexpr auto&	operator* (void)  { return *this; }
    constexpr auto&	operator++ (void) { return *this; }
    constexpr auto	operator++ (int)  { return *this; }
    inline auto&	operator= (const_reference v)
			    { _ctr.push_back (v); return *this; }
private:
    C&			_ctr;
};

template <class C>
inline constexpr auto back_inserter (C& ctr)
    { return back_insert_iterator<C> (ctr); }

//}}}-------------------------------------------------------------------
//{{{ front_insert_iterator

template <class C>
class front_insert_iterator : public insert_iterator<C> {
public:
    using const_reference = typename insert_iterator<C>::const_reference;
public:
    constexpr explicit	front_insert_iterator (C& ctr)
			    : insert_iterator<C> (ctr, ctr.begin()) {}
    constexpr auto&	operator* (void)  { return *this; }
    constexpr auto&	operator++ (void) { return *this; }
    constexpr auto	operator++ (int)  { return *this; }
    inline auto&	operator= (const_reference v)
			    { insert_iterator<C>::operator= (v); return *this; }
};

template <class C>
inline constexpr auto front_inserter (C& ctr)
    { return front_insert_iterator<C> (ctr); }

} // namespace cwiclo
//}}}-------------------------------------------------------------------
