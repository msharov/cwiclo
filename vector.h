// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "memblock.h"
#include "stream.h"

namespace cwiclo {

template <typename T>
class vector {
public:
    using value_type		= T;
    using pointer		= value_type*;
    using const_pointer		= const value_type*;
    using reference		= value_type&;
    using const_reference	= const value_type&;
    using iterator		= pointer;
    using const_iterator	= const_pointer;
    using size_type		= memblock::size_type;
    using difference_type	= memblock::difference_type;
    using initlist_t		= std::initializer_list<value_type>;
public:
    inline constexpr		vector (void)			: _data() { }
    inline explicit		vector (size_type n)		: _data() { uninitialized_default_construct_n (insert_hole(begin(),n), n); }
    inline			vector (size_type n, const T& v): _data() { uninitialized_fill_n (insert_hole(begin(),n), n, v); }
    inline			vector (const vector& v)	: _data() { uninitialized_copy_n (v.begin(), v.size(), iterator(_data.insert(_data.begin(),v.bsize()))); }
    inline			vector (const_iterator i1, const_iterator i2);
    template <size_type N>
    inline			vector (const T (&a)[N])	: vector (VectorRange(a)) {}
    inline			vector (initlist_t v);
    inline constexpr		vector (vector&& v)		: _data (move(v._data)) {}
    inline			~vector (void)			{ destroy_all(); }
    inline auto&		operator= (const vector& v)	{ assign (v); return *this; }
    inline auto&		operator= (vector&& v)		{ assign (move(v)); return *this; }
    inline auto&		operator= (initlist_t v)	{ assign (v); return *this; }
    bool			operator== (const vector& v) const	{ return equal_n (begin(), size(), v.begin(), v.size()); }
    inline bool			operator!= (const vector& v) const	{ return !(*this == v); }
    constexpr			operator const memlink& (void) const	{ return _data; }
    inline void			reserve (size_type n)		{ _data.reserve (n * sizeof(T)); }
    void			resize (size_type n);
    void			resize (size_type n, const_reference v);
    constexpr size_type		capacity (void) const		{ return _data.capacity() / sizeof(T);	}
    constexpr size_type		size (void) const		{ return _data.size() / sizeof(T);	}
    constexpr auto		bsize (void) const		{ return _data.size();			}
    constexpr size_type		max_size (void) const		{ return _data.max_size() / sizeof(T);	}
    [[nodiscard]]constexpr bool	empty (void) const		{ return _data.empty();			}
    constexpr auto		data (void)			{ return pointer_cast<T>(_data.data());	}
    constexpr auto		data (void) const		{ return pointer_cast<T>(_data.data());	}
    constexpr auto		begin (void)			{ return pointer_cast<T>(_data.begin());}
    constexpr auto		begin (void) const		{ return pointer_cast<T>(_data.begin());}
    constexpr auto		cbegin (void) const		{ return begin();			}
    constexpr auto		end (void)			{ return pointer_cast<T>(_data.end());	}
    constexpr auto		end (void) const		{ return pointer_cast<T>(_data.end());	}
    constexpr auto		cend (void) const		{ return end();				}
    constexpr auto		iat (size_type i)		{ assert (i <= size()); return begin() + i; }
    constexpr auto		iat (size_type i) const		{ assert (i <= size()); return begin() + i; }
    constexpr auto		ciat (size_type i) const	{ assert (i <= size()); return cbegin() + i; }
    constexpr auto		p2i (const_pointer p)		{ assert (begin() <= p && end() >= p); return begin() + (p - data()); }
    constexpr auto		p2i (const_pointer p) const	{ assert (begin() <= p && end() >= p); return begin() + (p - data()); }
    constexpr auto		iback (void)			{ return pointer_cast<T>(_data.end()-sizeof(T)); }
    constexpr auto		iback (void) const		{ return pointer_cast<T>(_data.end()-sizeof(T)); }
    constexpr auto&		at (size_type i)		{ assert (i < size()); return begin()[i]; }
    constexpr auto&		at (size_type i) const		{ assert (i < size()); return begin()[i]; }
    constexpr auto&		cat (size_type i) const		{ assert (i < size()); return cbegin()[i]; }
    constexpr auto&		operator[] (size_type i)	{ return at (i); }
    constexpr auto&		operator[] (size_type i) const	{ return at (i); }
    constexpr auto&		front (void)			{ assert (!empty()); return at(0); }
    constexpr auto&		front (void) const		{ assert (!empty()); return at(0); }
    constexpr auto&		back (void)			{ assert (!empty()); return *iback(); }
    constexpr auto&		back (void) const		{ assert (!empty()); return *iback(); }
    inline constexpr void	clear (void)			{ destroy_all(); _data.clear(); }
    inline constexpr void	swap (vector&& v)		{ _data.swap (move(v._data)); }
    inline void			deallocate (void)		{ assert (!is_linked()); destroy_all(); _data.deallocate(); }
    inline void			shrink_to_fit (void)		{ _data.shrink_to_fit(); }
    inline void			assign (const vector& v)	{ assign (v.begin(), v.end()); }
    inline void			assign (vector&& v)		{ swap (v); }
    inline void			assign (const_iterator i1, const_iterator i2);
    inline void			assign (size_type n, const T& v)	{ resize (n); fill_n (begin(), n, v); }
    inline void			assign (initlist_t v)			{ assign (v.begin(), v.end()); }
    inline auto			insert (const_iterator ip)		{ return construct_at (insert_hole (ip, 1)); }
    inline auto			insert (const_iterator ip, const T& v)	{ return emplace (ip, v); }
    inline auto			insert (const_iterator ip, T&& v)	{ return emplace (ip, move(v)); }
    auto			insert (const_iterator ip, size_type n, const T& v);
    auto			insert (const_iterator ip, const_iterator i1, const_iterator i2);
    inline auto			insert (const_iterator ip, initlist_t v)	{ return insert (ip, v.begin(), v.end()); }
    template <typename... Args>
    inline auto			emplace (const_iterator ip, Args&&... args)	{ return construct_at (insert_hole(ip,1), forward<Args>(args)...); }
    template <typename... Args>
    inline auto&		emplace_back (Args&&... args)			{ return *emplace(end(), forward<Args>(args)...); }
    auto			erase (const_iterator ep, size_type n = 1);
    inline auto			erase (const_iterator ep1, const_iterator ep2)	{ assert (ep1 <= ep2); return erase (ep1, ep2 - ep1); }
    auto			replace (const_iterator rp1, const_iterator rp2, const_iterator i1, const_iterator i2)
									{ return insert (erase (rp1, rp2), i1, i2); }
    inline auto&		push_back (const T& v)			{ return emplace_back (v); }
    inline auto&		push_back (T&& v)			{ return emplace_back (move(v)); }
    inline auto&		push_back (void)			{ return emplace_back(); }
    inline auto			append (const T& v)			{ return &push_back(v); }
    inline auto			append (T&& v)				{ return &push_back(move(v)); }
    inline auto			append (const_iterator i1, const_iterator i2)	{ return insert (end(), i1, i2); }
    inline auto			append (size_type n, const T& v)	{ return insert (end(), n, v); }
    inline auto			append (initlist_t v)			{ return append (v.begin(), v.end()); }
    inline void			pop_back (void)				{ auto ns = _data.size() - sizeof(T); auto b = iback(); _data.memlink::resize (ns); destroy_at (b); }
    inline void			manage (pointer p, size_type n)		{ _data.manage (p, n * sizeof(T)); }
    inline constexpr bool	is_linked (void) const			{ return !_data.capacity(); }
    inline constexpr void	unlink (void)				{ _data.unlink(); }
    inline void			copy_link (void);
    inline constexpr void	link (pointer p, size_type n)		{ _data.link (p, n*sizeof(T)); }
    inline constexpr void	link (const vector& v)			{ _data.link (v); }
    inline constexpr void	link (pointer f, pointer l)		{ link (f, l-f); }
    void			read (istream& is) {
				    if constexpr (stream_align<T>::value <= stream_align<size_type>::value && is_trivially_copyable<T>::value)
					return _data.read (is, sizeof(T));
				    size_type n; is >> n;
				    if constexpr (stream_align<T>::value > stream_align<size_type>::value)
					is >> ios::talign<T>();
				    if constexpr (is_trivially_copyable<T>::value) {
					assert (is.remaining() >= n*sizeof(T));
					const T* d = is.ptr<T>();
					assign (d, d+n);
					is.skip (n*sizeof(T));
				    } else {
					resize (n);
					for (auto& i : *this)
					    is >> i;
				    }
				    if constexpr (stream_align<T>::value < stream_align<size_type>::value)
					is >> ios::talign<size_type>();
				}
    template <typename Stm>
    inline constexpr void	write (Stm& os) const {
				    if constexpr (stream_align<T>::value <= stream_align<size_type>::value && is_trivially_copyable<T>::value)
					return _data.write (os, sizeof(T));
				    os.write_array (data(), size());
				}
protected:
    inline auto			insert_hole (const_iterator ip, size_type n)
				    { return iterator (_data.insert (memblock::const_iterator(ip), n*sizeof(T))); }
    constexpr void		destroy_all (void)
				    { if (!is_linked()) destroy (begin(), end()); }
private:
    memblock			_data;	///< Raw element data, consecutively stored.
};

//----------------------------------------------------------------------

template <typename T>
class vector_view {
public:
    using vector_t		= vector<T>;
    using value_type		= typename vector_t::value_type;
    using pointer		= typename vector_t::pointer;
    using const_pointer		= typename vector_t::const_pointer;
    using reference		= typename vector_t::pointer;
    using const_reference	= typename vector_t::const_pointer;
    using iterator		= typename vector_t::pointer;
    using const_iterator	= typename vector_t::const_pointer;
    using size_type		= typename vector_t::size_type;
    using difference_type	= typename vector_t::difference_type;
    using initlist_t		= typename vector_t::initlist_t;
public:
    inline constexpr		vector_view (void)			: _data() { }
    inline constexpr		vector_view (const vector_view& v)	: _data (v._data) {}
    inline constexpr		vector_view (vector_view&& v)		: _data (move(v._data)) {}
    inline constexpr		vector_view (const vector_t& v)		: _data (begin(v), size(v)) {}
    inline constexpr		vector_view (const_iterator i1, const_iterator i2)	: _data (i1, distance(i1,i2)) {}
    template <size_type N>
    inline constexpr		vector_view (const T (&a)[N])		: _data (begin(a), size(a)) {}
    inline constexpr		vector_view (initlist_t v)		: _data (begin(v), size(v)) {}
    inline constexpr auto&	operator= (const vector_view& v)	{ assign (v); return *this; }
    inline constexpr auto&	operator= (vector_view&& v)		{ assign (move(v)); return *this; }
    inline constexpr auto&	operator= (initlist_t v)		{ assign (v); return *this; }
    bool			operator== (const vector_view& v) const	{ return equal_n (begin(), size(), v.begin(), v.size()); }
    inline bool			operator!= (const vector_view& v) const	{ return !(*this == v); }
    bool			operator== (const vector_t& v) const	{ return equal_n (begin(), size(), v.begin(), v.size()); }
    inline bool			operator!= (const vector_t& v) const	{ return !(*this == v); }
    constexpr			operator const cmemlink& (void) const	{ return _data; }
    inline			operator const vector_t& (void) const	{ return reinterpret_cast<const vector_t&>(*this); }
    constexpr size_type		capacity (void) const			{ return 0; }
    constexpr size_type		size (void) const			{ return _data.size() / sizeof(T);	}
    constexpr auto		bsize (void) const			{ return _data.size();			}
    constexpr size_type		max_size (void) const			{ return _data.max_size() / sizeof(T);	}
    [[nodiscard]]constexpr bool	empty (void) const			{ return _data.empty();			}
    constexpr auto		data (void) const			{ return pointer_cast<T>(_data.data());	}
    constexpr auto		begin (void) const			{ return pointer_cast<T>(_data.begin());}
    constexpr auto		cbegin (void) const			{ return begin();			}
    constexpr auto		end (void) const			{ return pointer_cast<T>(_data.end());	}
    constexpr auto		cend (void) const			{ return end();				}
    constexpr auto		iat (size_type i) const			{ assert (i <= size()); return begin() + i; }
    constexpr auto		ciat (size_type i) const		{ assert (i <= size()); return cbegin() + i; }
    constexpr auto		p2i (const_pointer p) const		{ assert (begin() <= p && end() >= p); return begin() + (p - data()); }
    constexpr auto		iback (void) const			{ return pointer_cast<T>(_data.end()-sizeof(T)); }
    constexpr auto&		at (size_type i) const			{ assert (i < size()); return begin()[i]; }
    constexpr auto&		cat (size_type i) const			{ assert (i < size()); return cbegin()[i]; }
    constexpr auto&		operator[] (size_type i) const		{ return at (i); }
    constexpr auto&		front (void) const			{ assert (!empty()); return at(0); }
    constexpr auto&		back (void) const			{ assert (!empty()); return *iback(); }
    inline constexpr void	swap (vector_view&& v)			{ _data.swap (move(v._data)); }
    inline void			assign (const vector_t& v)		{ assign (v.begin(), v.end()); }
    inline void			assign (vector_view&& v)		{ swap (v); }
    inline void			assign (const_iterator i1, const_iterator i2)	{ _data.link (i1, distance(i1,i2)); }
    inline void			assign (initlist_t v)			{ assign (ARRAY_BLOCK(v)); }
    inline constexpr bool	is_linked (void) const			{ return true; }
    inline constexpr void	unlink (void)				{ _data.unlink(); }
    inline constexpr void	link (const_pointer p, size_type n)	{ _data.link (p, n*sizeof(T)); }
    inline constexpr void	link (const vector_view& v)		{ _data.link (v._data); }
    inline constexpr void	link (const vector_t& v)		{ _data.link (begin(v), size(v)); }
    inline constexpr void	link (const_pointer f, const_pointer l)	{ link (f, distance(f,l)); }
    void			read (istream& is)			{ _data.link_read (is, sizeof(T)); }
    template <typename Stm>
    inline constexpr void	write (Stm& os) const			{ os.write_array (this->data(), this->size()); }
private:
    cmemlink			_data;
};

//{{{ cmemlink stream functions ----------------------------------------
// These are here because streams need cmemlink

} // namespace cwiclo
STREAM_ALIGN (cmemlink, stream_align<cmemlink::size_type>::value)
namespace cwiclo {

constexpr void cmemlink::write (sstream& os, size_type elsize) const
{
    auto sz = size();
    if (sz)
	sz += zero_terminated();
    os << size_type(sz/elsize);
    os.write (data(), sz);
    os.align (stream_align<size_type>::value);
}

template <typename T> struct stream_align<vector<T>> {
    static constexpr const streamsize value = stream_align<memblock::size_type>::value;
};

template <typename T> struct stream_align<vector_view<T>> {
    static constexpr const streamsize value = stream_align<memblock::size_type>::value;
};

//}}}-------------------------------------------------------------------
//{{{ vector out-of-lines

/// Copies range [\p i1, \p i2]
template <typename T>
vector<T>::vector (const_iterator i1, const_iterator i2)
:_data()
{
    const auto n = i2-i1;
    uninitialized_copy_n (i1, n, insert_hole(begin(),n));
}

template <typename T>
vector<T>::vector (initlist_t v)
:_data()
{
    uninitialized_copy_n (v.begin(), v.size(), insert_hole(begin(),v.size()));
}

/// Resizes the vector to contain \p n elements.
template <typename T>
void vector<T>::resize (size_type n)
{
    reserve (n);
    auto ihfirst = end(), ihlast = begin()+n;
    destroy (ihlast, ihfirst);
    uninitialized_default_construct (ihfirst, ihlast);
    _data.memlink::resize (n*sizeof(T));
}

/// Resizes the vector to contain \p n elements made from \p v.
template <typename T>
void vector<T>::resize (size_type n, const_reference v)
{
    reserve (n);
    auto ihfirst = end(), ihlast = begin()+n;
    destroy (ihlast, ihfirst);
    uninitialized_fill (ihfirst, ihlast, v);
    _data.memlink::resize (n*sizeof(T));
}

/// Copies the range [\p i1, \p i2]
template <typename T>
void vector<T>::assign (const_iterator i1, const_iterator i2)
{
    assert (i1 <= i2);
    resize (i2 - i1);
    copy (i1, i2, begin());
}

/// Inserts \p n elements with value \p v at offsets \p ip.
template <typename T>
auto vector<T>::insert (const_iterator ip, size_type n, const T& v)
{
    auto ih = insert_hole (ip, n);
    uninitialized_fill_n (ih, n, v);
    return ih;
}

/// Inserts range [\p i1, \p i2] at offset \p ip.
template <typename T>
auto vector<T>::insert (const_iterator ip, const_iterator i1, const_iterator i2)
{
    assert (i1 <= i2);
    auto n = i2 - i1;
    auto ih = insert_hole (ip, n);
    uninitialized_copy_n (i1, n, ih);
    return ih;
}

/// Removes \p count elements at offset \p ep.
template <typename T>
auto vector<T>::erase (const_iterator cep, size_type n)
{
    auto ep = p2i (cep);
    destroy_n (ep, n);
    return iterator (_data.erase (memblock::iterator(ep), n * sizeof(T)));
}

template <typename T>
void vector<T>::copy_link (void)
{
    if constexpr (is_trivial<T>::value)
	_data.copy_link();
    else
	assign (begin(), end());
}

//}}}-------------------------------------------------------------------

} // namespace cwiclo
