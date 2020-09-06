// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "memblock.h"
#include "stream.h"

namespace cwiclo {

class string_view;

class string : public memblock {
public:
    inline constexpr	string (void)					: memblock() { set_zero_terminated(); }
    inline constexpr	string (const_pointer s, size_type len, size_type cap)  : memblock (s,len,cap,true) {}
    inline constexpr	string (pointer s, size_type len, size_type cap): memblock (s,len,cap,true) {}
    inline		string (const_pointer s, size_type len)		: memblock (s,len,true) {}
    inline		string (const_pointer s1, const_pointer s2)	: string (s1,s2-s1) {}
    inline		string (const_pointer s)			: string (s,__builtin_strlen(s)) {}
    inline constexpr	string (string&& s)				: memblock (move(s)) {}
    inline		string (const string& s)			: string (s.data(), s.size()) {}
    inline		string (const cmemlink& s)			: string (s.data(), s.size()) {}
    constexpr auto	c_str (void) const				{ assert ((!end() || !*end()) && "This string is linked to data that is not 0-terminated. This may cause serious security problems. Please assign the data instead of linking."); return data(); }
    constexpr auto&	back (void) const				{ return at(size()-1); }
    constexpr auto&	back (void)					{ return at(size()-1); }
    constexpr auto	wbegin (void) const				{ return utf8::in (begin()); }
    constexpr auto	wend (void) const				{ return utf8::in (end()); }
    constexpr auto	wiat (size_type i) const			{ return wbegin()+i; }
    constexpr size_type	length (void) const				{ return wend()-wbegin(); }
    constexpr size_type	capacity (void) const				{ return memblock::capacity()-1; }
    inline void		push_back (char c)				{ resize(size()+1); back() = c; }
    auto		insert (const_iterator ip, value_type c, size_type n = 1)	{ return fill_n (memblock::insert (ip, n), n, c); }
    inline auto		insert (const_iterator ip, const_pointer s, size_type n)	{ return memblock::insert (ip, s, n); }
    inline auto		insert (const_iterator ip, const string& s)			{ return insert (ip, s.c_str(), s.size()); }
    inline auto		insert (const_iterator ip, const_pointer s)			{ return insert (ip, s, strlen(s)); }
    inline auto		insert (const_iterator ip, const_pointer f, const_iterator l)	{ return insert (ip, f, l-f); }
    iterator		insert (const_iterator ip, char32_t c);
    int			insertv (const_iterator ip, const char* fmt, va_list args);
    int			insertf (const_iterator ip, const char* fmt, ...) PRINTFARGS(3,4);
			using memblock::append;
    inline auto		append (char c, size_type n = 1)		{ return insert (end(), c, n); }
    inline auto	   	append (const_pointer s)			{ return append (s, strlen(s)); }
    inline auto		append (const_iterator i1, const_iterator i2)	{ assert (i1<=i2); return append (i1, i2-i1); }
    inline auto		append (char32_t c)				{ return insert (end(), c); }
    int			appendv (const char* fmt, va_list args);
    int			appendf (const char* fmt, ...) PRINTFARGS(2,3);
			using memblock::assign;
    inline void		assign (value_type c)				{ assign (&c, 1); }
    inline void	    	assign (const_pointer s)			{ assign (s, strlen(s)); }
    inline void		assign (const_iterator i1, const_iterator i2)	{ assert (i1<=i2); assign (i1, i2-i1); }
    void		assign (char32_t c)				{ clear(); append (c); }
    int			assignv (const char* fmt, va_list args);
    int			assignf (const char* fmt, ...) PRINTFARGS(2,3);
    inline static auto	create_from_file (const char* filename)		{ string r; r.read_file (filename); return r; }
    template <typename... Args>
    inline static auto	createf (const char* fmt, Args&&... args)	{ string r; r.assignf (fmt, forward<Args>(args)...); return r; }
    static int		compare (const_iterator f1, const_iterator l1, const_iterator f2, const_iterator l2);
    auto		compare (const string& s) const			{ return compare (begin(), end(), s.begin(), s.end()); }
    auto		compare (const_pointer s) const			{ return compare (begin(), end(), s, s + strlen(s)); }
    auto		compare (char c) const				{ return compare (begin(), end(), &c, &c+1); }
    inline constexpr void	swap (string&& s)			{ memblock::swap (move(s)); }
    inline constexpr auto&	operator= (string&& s)			{ memblock::operator= (move(s)); return *this; }
    inline auto&	operator= (const string& s)			{ assign (s); return *this; }
    inline auto&	operator= (const cmemlink& s)			{ assign (s); return *this; }
    inline auto&	operator= (const_pointer s)			{ assign (s); return *this; }
    inline auto&	operator= (char c)				{ assign (c); return *this; }
    inline auto&	operator= (char32_t c)				{ assign (c); return *this; }
    inline auto&	operator+= (const string& s)			{ append (s); return *this; }
    inline auto&	operator+= (const cmemlink& s)			{ append (s); return *this; }
    inline auto&	operator+= (const_pointer s)			{ append (s); return *this; }
    inline auto&	operator+= (char c)				{ push_back (c); return *this; }
    inline auto&	operator+= (char32_t c)				{ append (c); return *this; }
    inline auto		operator+ (const string& s) const		{ auto r (*this); return r += s; }
    inline auto		operator+ (const cmemlink& s) const		{ auto r (*this); return r += s; }
    inline auto		operator+ (const_pointer s) const		{ auto r (*this); return r += s; }
    inline auto		operator+ (char c) const			{ auto r (*this); return r += c; }
    inline auto		operator+ (char32_t c) const			{ auto r (*this); return r += c; }
    inline bool		operator== (const string& s) const		{ return memblock::operator== (s); }
    bool		operator== (const_pointer s) const;
    constexpr bool	operator== (char c) const			{ return size() == 1 && c == at(0); }
    inline bool		operator!= (const string& s) const		{ return !operator== (s); }
    inline bool		operator!= (const_pointer s) const		{ return !operator== (s); }
    constexpr bool	operator!= (char c) const			{ return !operator== (c); }
    inline bool		operator< (const string& s) const		{ return 0 > compare (s); }
    inline bool		operator< (const_pointer s) const		{ return 0 > compare (s); }
    inline bool		operator< (char c) const			{ return 0 > compare (c); }
    inline bool		operator> (const string& s) const		{ return 0 < compare (s); }
    inline bool		operator> (const_pointer s) const		{ return 0 < compare (s); }
    inline bool		operator> (char c) const			{ return 0 < compare (c); }
    inline bool		operator<= (const string& s) const		{ return 0 >= compare (s); }
    inline bool		operator<= (const_pointer s) const		{ return 0 >= compare (s); }
    inline bool		operator<= (char c) const			{ return 0 >= compare (c); }
    inline bool		operator>= (const string& s) const		{ return 0 <= compare (s); }
    inline bool		operator>= (const_pointer s) const		{ return 0 <= compare (s); }
    inline bool		operator>= (char c) const			{ return 0 <= compare (c); }
    inline constexpr	operator const string_view& (void) const;
    inline auto		erase (const_iterator ep, size_type n = 1)	{ return memblock::erase (ep, n); }
    inline auto		erase (const_iterator f, const_iterator l)	{ assert (f<=l); return erase (f, l-f); }
    inline constexpr void shrink (size_type sz)	{ memblock::shrink(sz); if (auto z = end(); z) { assert (!is_linked() && "modifying a const linked string"); *z = 0; }}
    constexpr void	pop_back (void)		{ shrink (size()-1); }
    constexpr void	clear (void)		{ shrink (0); }

    template <typename Stm> // Override cmemlink's write to support lack of terminating 0
    inline constexpr void write (Stm& os) const	{ os.write_string (data(), size()); }

    inline auto		replace (const_iterator f, const_iterator l, const_pointer s, size_type slen)	{ return memblock::replace (f, l-f, s, slen); }
    inline auto		replace (const_iterator f, const_iterator l, const_pointer s)			{ return replace (f, l, s, strlen(s)); }
    inline auto		replace (const_iterator f, const_iterator l, const_pointer i1,const_pointer i2)	{ return replace (f, l, i1, i2-i1); }
    inline auto		replace (const_iterator f, const_iterator l, const string& s)			{ return replace (f, l, s.begin(), s.end()); }
    iterator		replace (const_iterator f, const_iterator l, value_type c, size_type n = 1);

    constexpr auto	find (char c, const_iterator fi) const		{ return fi ? const_iterator (__builtin_strchr (fi, c)) : fi; }
    constexpr auto	find (const_pointer s, const_iterator fi) const	{ return fi ? const_iterator (__builtin_strstr (fi, s)) : fi; }
    constexpr auto	find (const string& s, const_iterator fi) const	{ return find (s.c_str(), fi); }
    constexpr auto	find (char c) const				{ return find (c, begin()); }
    constexpr auto	find (const_pointer s) const			{ return find (s, begin()); }
    constexpr auto	find (const string& s) const			{ return find (s, begin()); }

    constexpr auto	find (char c, const_iterator fi)		{ return UNCONST_MEMBER_FN (find,c,fi); }
    constexpr auto	find (const_pointer s, const_iterator fi)	{ return UNCONST_MEMBER_FN (find,s,fi); }
    constexpr auto	find (const string& s, const_iterator fi)	{ return UNCONST_MEMBER_FN (find,s,fi); }
    constexpr auto	find (char c)					{ return UNCONST_MEMBER_FN (find,c); }
    constexpr auto	find (const_pointer s)				{ return UNCONST_MEMBER_FN (find,s); }
    constexpr auto	find (const string& s)				{ return UNCONST_MEMBER_FN (find,s); }

    inline auto		rfind (char c, const_iterator fi) const	{ return fi ? const_iterator (memrchr (begin(), c, fi-begin())) : fi; }
    const_iterator	rfind (const_pointer s, const_iterator fi) const;
    inline auto		rfind (const string& s, const_iterator fi) const	{ return rfind (s.c_str(), fi); }
    inline auto		rfind (char c) const					{ return rfind (c, end()); }
    inline auto		rfind (const_pointer s) const				{ return rfind (s, end()); }
    inline auto		rfind (const string& s) const				{ return rfind (s, end()); }

    inline auto		rfind (char c, const_iterator fi)			{ return UNCONST_MEMBER_FN (rfind,c,fi); }
    inline auto		rfind (const_pointer s, const_iterator fi)		{ return UNCONST_MEMBER_FN (rfind,s,fi); }
    inline auto		rfind (const string& s, const_iterator fi)		{ return UNCONST_MEMBER_FN (rfind,s,fi); }
    inline auto		rfind (char c)						{ return UNCONST_MEMBER_FN (rfind,c); }
    inline auto		rfind (const_pointer s)					{ return UNCONST_MEMBER_FN (rfind,s); }
    inline auto		rfind (const string& s)					{ return UNCONST_MEMBER_FN (rfind,s); }

    constexpr auto	find_first_of (const_pointer s, const_iterator fi)const	{ if (!fi) return fi; auto rsz = __builtin_strcspn (fi, s); return fi+rsz < end() ? fi+rsz : nullptr; }
    constexpr auto	find_first_of (const string& s, const_iterator fi)const	{ return find_first_of (s.c_str(), fi); }
    constexpr auto	find_first_of (const_pointer s) const			{ return find_first_of (s, begin()); }
    constexpr auto	find_first_of (const string& s) const			{ return find_first_of (s, begin()); }
    constexpr auto	find_first_not_of (const_pointer s, const_iterator fi) const	{ if (!fi) return fi; auto rsz = __builtin_strspn (fi, s); return fi+rsz < end() ? fi+rsz : nullptr; }
    constexpr auto	find_first_not_of (const string& s, const_iterator fi) const	{ return find_first_not_of (s.c_str(), fi); }
    constexpr auto	find_first_not_of (const_pointer s) const		{ return find_first_not_of (s, begin()); }
    constexpr auto	find_first_not_of (const string& s) const		{ return find_first_not_of (s, begin()); }

    constexpr auto	find_first_of (const_pointer s, const_iterator fi)	{ return UNCONST_MEMBER_FN (find_first_of,s,fi); }
    constexpr auto	find_first_of (const string& s, const_iterator fi)	{ return UNCONST_MEMBER_FN (find_first_of,s,fi); }
    constexpr auto	find_first_of (const_pointer s)				{ return UNCONST_MEMBER_FN (find_first_of,s); }
    constexpr auto	find_first_of (const string& s)				{ return UNCONST_MEMBER_FN (find_first_of,s); }
    constexpr auto	find_first_not_of (const_pointer s, const_iterator fi)	{ return UNCONST_MEMBER_FN (find_first_not_of,s,fi); }
    constexpr auto	find_first_not_of (const string& s, const_iterator fi)	{ return UNCONST_MEMBER_FN (find_first_not_of,s,fi); }
    constexpr auto	find_first_not_of (const_pointer s)			{ return UNCONST_MEMBER_FN (find_first_not_of,s); }
    constexpr auto	find_first_not_of (const string& s)			{ return UNCONST_MEMBER_FN (find_first_not_of,s); }

    constexpr bool	starts_with (const string& s) const			{ return size() >= s.size() && 0 == strncmp (begin(), s.begin(), s.size()); }
    constexpr bool	starts_with (const_pointer s) const			{ auto l = strlen(s); return size() >= l && 0 == strncmp (begin(), s, l); }
    constexpr bool	starts_with (char c) const				{ return size() >= 1 && at(0) == c; }

    constexpr bool	ends_with (const string& s) const			{ return size() >= s.size() && 0 == strncmp (end()-s.size(), s.begin(), s.size()); }
    constexpr bool	ends_with (const_pointer s) const			{ auto l = strlen(s); return size() >= l && 0 == strncmp (end()-l, s, l); }
    constexpr bool	ends_with (char c) const				{ return size() >= 1 && back() == c; }
};

//----------------------------------------------------------------------

class string_view : public cmemlink {
public:
    inline constexpr		string_view (void)		: cmemlink () { set_zero_terminated(); }
    inline constexpr		string_view (const_pointer s, size_type len)	: cmemlink (s, len, true) {}
    inline constexpr		string_view (const_pointer s1, const_pointer s2): string_view (s1, s2-s1) {}
    inline constexpr		string_view (const_pointer s)	: string_view (s, __builtin_strlen(s)) {}
    inline constexpr		string_view (string_view&& s)	: cmemlink (move(s)) {}
    inline constexpr		string_view (const string_view& s)	: cmemlink (s) {}
    inline constexpr		string_view (const string& s)	: cmemlink (s) {}
    inline constexpr void	swap (string_view&& s)		{ cmemlink::swap (move(s)); }
    inline constexpr auto&	operator= (const string& s)	{ cmemlink::operator= (s); return *this; }
    inline constexpr auto&	operator= (string_view&& s)	{ cmemlink::operator= (move(s)); return *this; }

    inline constexpr auto&	str (void) const		{ return static_cast<const string&>(static_cast<const cmemlink&>(*this)); }
    inline constexpr		operator const string& (void) const	{ return str(); }

    inline constexpr auto	c_str (void) const		{ assert ((!end() || !*end()) && "This string is linked to data that is not 0-terminated. This may cause serious security problems. Please assign the data instead of linking."); return data(); }
    inline constexpr auto&	back (void) const		{ return at(size()-1); }

    inline static int	compare (const_iterator f1, const_iterator l1, const_iterator f2, const_iterator l2)
								{ return string::compare (f1,l1,f2,l2); }
    inline auto		compare (const string_view& s) const	{ return compare (begin(), end(), s.begin(), s.end()); }
    inline auto		compare (const string& s) const		{ return compare (begin(), end(), s.begin(), s.end()); }
    inline auto		compare (const_pointer s) const		{ return compare (begin(), end(), s, s + strlen(s)); }

    inline bool		operator== (const string& s) const	{ return str() == s; }
    inline bool		operator== (const_pointer s) const	{ return str() == s; }
    constexpr bool	operator== (char c) const		{ return str() == c; }
    inline bool		operator!= (const string& s) const	{ return str() != s; }
    inline bool		operator!= (const_pointer s) const	{ return str() != s; }
    constexpr bool	operator!= (char c) const		{ return str() != c; }
    inline bool		operator< (const string& s) const	{ return str() < s; }
    inline bool		operator< (const_pointer s) const	{ return str() < s; }
    inline bool		operator< (char c) const		{ return str() < c; }
    inline bool		operator> (const string& s) const	{ return str() > s; }
    inline bool		operator> (const_pointer s) const	{ return str() > s; }
    inline bool		operator> (char c) const		{ return str() > c; }
    inline bool		operator<= (const string& s) const	{ return str() <= s; }
    inline bool		operator<= (const_pointer s) const	{ return str() <= s; }
    inline bool		operator<= (char c) const		{ return str() <= c; }
    inline bool		operator>= (const string& s) const	{ return str() >= s; }
    inline bool		operator>= (const_pointer s) const	{ return str() >= s; }
    inline bool		operator>= (char c) const		{ return str() >= c; }

    constexpr auto	find (char c, const_iterator fi) const	{ return str().find (c, fi); }
    constexpr auto	find (const_pointer s, const_iterator fi) const		{ return str().find (s, fi); }
    constexpr auto	find (const string& s, const_iterator fi) const		{ return str().find (s, fi); }
    constexpr auto	find (char c) const					{ return str().find (c); }
    constexpr auto	find (const_pointer s) const				{ return str().find (s); }
    constexpr auto	find (const string& s) const				{ return str().find (s); }

    inline auto		rfind (char c, const_iterator fi) const	{ return str().rfind (c, fi); }
    inline auto		rfind (const_pointer s, const_iterator fi) const	{ return str().rfind (s, fi); }
    inline auto		rfind (const string& s, const_iterator fi) const	{ return str().rfind (s, fi); }
    inline auto		rfind (char c) const					{ return str().rfind (c); }
    inline auto		rfind (const_pointer s) const				{ return str().rfind (s); }
    inline auto		rfind (const string& s) const				{ return str().rfind (s); }

    constexpr auto	find_first_of (const_pointer s, const_iterator fi)const	{ return str().find_first_of (s,fi); }
    constexpr auto	find_first_of (const string& s, const_iterator fi)const	{ return str().find_first_of (s,fi); }
    constexpr auto	find_first_of (const_pointer s) const			{ return str().find_first_of (s); }
    constexpr auto	find_first_of (const string& s) const			{ return str().find_first_of (s); }
    constexpr auto	find_first_not_of (const_pointer s, const_iterator fi) const	{ return str().find_first_not_of (s,fi); }
    constexpr auto	find_first_not_of (const string& s, const_iterator fi) const	{ return str().find_first_not_of (s,fi); }
    constexpr auto	find_first_not_of (const_pointer s) const		{ return str().find_first_not_of (s); }
    constexpr auto	find_first_not_of (const string& s) const		{ return str().find_first_not_of (s); }

    constexpr bool	starts_with (const string& s) const			{ return str().starts_with (s); }
    constexpr bool	starts_with (const_pointer s) const			{ return str().starts_with (s); }
    constexpr bool	starts_with (char c) const				{ return str().starts_with (c); }

    constexpr bool	ends_with (const string& s) const			{ return str().ends_with (s); }
    constexpr bool	ends_with (const_pointer s) const			{ return str().ends_with (s); }
    constexpr bool	ends_with (char c) const				{ return str().ends_with (c); }

    void		resize (size_type sz) = delete;
    void		shrink (void) = delete;
    void		clear (void) = delete;

    template <typename Stm> // Override cmemlink's write to support lack of terminating 0
    inline constexpr void write (Stm& os) const { os.write_string (begin(), size()); }

    // istream read can be done more efficiently by constructing the string_view directly
    inline static constexpr string_view create_from_stream (istream& is) {
	auto ssz = is.read<uint32_t>();
	auto scp = is.ptr<char>();
	is.skip (ceilg (ssz,sizeof(ssz)));
	if (ssz)
	    --ssz;
	else
	    --scp;
	return string_view (scp, ssz);
    }
};

//----------------------------------------------------------------------

// Here because it needs definition of string_view
constexpr string::operator const string_view& (void) const
    { return static_cast<const string_view&>(static_cast<const cmemlink&>(*this)); }

//----------------------------------------------------------------------

} // namespace cwiclo
