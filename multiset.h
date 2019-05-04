// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "vector.h"

namespace cwiclo {

template <typename T>
class multiset : public vector<T> {
public:
    using vecbase = vector<T>;
    using typename vecbase::value_type;
    using typename vecbase::reference;
    using typename vecbase::const_reference;
    using typename vecbase::pointer;
    using typename vecbase::const_pointer;
    using typename vecbase::iterator;
    using typename vecbase::const_iterator;
    using typename vecbase::size_type;
    using typename vecbase::difference_type;
    using typename vecbase::initlist_t;
public:
			using vecbase::vector;
			using vecbase::insert;
			using vecbase::erase;
    inline		multiset (const multiset& v)	: vecbase(v) {}
    inline		multiset (const vecbase& v)	: vecbase(v) { sort(*this); }
    template <size_type N>
    inline		multiset (const T (&a)[N])	: vecbase(a) { sort(*this); }
    inline		multiset (initlist_t v)		: vecbase(v) { sort(*this); }
    inline constexpr	multiset (multiset&& v)		: vecbase(move(v)) {}
    inline		multiset (const_iterator i1, const_iterator i2)	: vecbase(i1,i2) { sort(*this); }
    inline void		assign (const_iterator i1, const_iterator i2)	{ vecbase::assign(i1,i2); sort(*this); }
    inline void		assign (const multiset& v)	{ vecbase::assign(v); }
    inline void		assign (const vecbase& v)	{ vecbase::assign(v); sort(*this); }
    inline void		assign (multiset&& v)		{ vecbase::assign(move(v)); }
    inline void		assign (vecbase&& v)		{ vecbase::assign(move(v)); sort(*this); }
    inline void		assign (size_type n,const T& v)	{ vecbase::assign(n,v); }
    inline void		assign (initlist_t v)		{ vecbase::assign(v); sort(*this); }
    inline auto&	operator= (const multiset& v)	{ assign (v); return *this; }
    inline auto&	operator= (const vecbase& v)	{ assign (v); return *this; }
    inline auto&	operator= (multiset&& v)	{ assign (move(v)); return *this; }
    inline auto&	operator= (vecbase&& v)		{ assign (move(v)); return *this; }
    inline auto&	operator= (initlist_t v)	{ assign (v); return *this; }
    template <typename U>
    constexpr auto	find (const U& v) const		{ return binary_search (this->begin(), this->end(), v); }
    template <typename U>
    constexpr auto	lower_bound (const U& v) const	{ return ::cwiclo::lower_bound (this->begin(), this->end(), v); }
    template <typename U>
    constexpr auto	upper_bound (const U& v) const	{ return ::cwiclo::upper_bound (this->begin(), this->end(), v); }
    template <typename U>
    constexpr auto	find (const U& v)		{ return UNCONST_MEMBER_FN (find,v); }
    template <typename U>
    constexpr auto	lower_bound (const U& v)	{ return UNCONST_MEMBER_FN (lower_bound,v); }
    template <typename U>
    constexpr auto	upper_bound (const U& v)	{ return UNCONST_MEMBER_FN (upper_bound,v); }
    auto		insert (const_reference v)	{ return vecbase::insert (lower_bound(v), v); }
    auto		insert (T&& v)			{ return vecbase::insert (lower_bound(v), move(v)); }
    inline void		insert (const_iterator i1, const_iterator i2)	{ for (; i1 < i2; ++i1) insert (*i1); }
    inline void		insert (initlist_t v)		{ insert (v.begin(), v.end()); }
    template <typename... Args>
    auto		emplace (Args&&... args) noexcept;
    template <typename... Args>
    auto		emplace_hint (const_iterator ip, Args&&... args)	{ return vecbase::emplace (ip, forward<Args>(args)...); }
    auto		erase (const_reference v) noexcept {
			    auto l = lower_bound (v), u = l;
			    while (*u == v) ++u;
			    return erase (l, u);
			}
};

template <typename T>
template <typename... Args>
auto multiset<T>::emplace (Args&&... args) noexcept
{
    auto e = &vecbase::emplace_back (forward<Args>(args)...),
	iend = this->end(), ilast = iend-1,
	ip = ::cwiclo::lower_bound (this->begin(), ilast, *e);
    return rotate (ip, ilast, iend);
}

} // namespace cwiclo
