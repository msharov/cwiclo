// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "string.h"
#include <stdarg.h>

namespace cwiclo {

string::iterator string::insert (const_iterator ip, char32_t c)
{
    auto ioi = utf8::out (memblock::insert (ip, utf8::obytes(c)));
    *ioi++ = c;
    return ioi.base();
}

int string::insertv (const_iterator cip, const char* fmt, va_list args)
{
    auto ip = p2i (cip);
    const char c = (ip < end() ? *ip : 0);
    for (int wcap = 0;;) {
	va_list args2;
	va_copy (args2, args);
	auto rv = vsnprintf (ip, wcap, fmt, args2);
	if (rv <= wcap) {
	    ip[rv] = c;
	    return rv;
	}
	ip = memblock::insert (ip, rv);
	wcap = rv+1;
    }
}

int string::insertf (const_iterator ip, const char* fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    auto rv = insertv (ip, fmt, args);
    va_end (args);
    return rv;
}

int string::appendv (const char* fmt, va_list args)
{
    return insertv (end(), fmt, args);
}

int string::appendf (const char* fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    auto rv = appendv (fmt, args);
    va_end (args);
    return rv;
}

int string::assignv (const char* fmt, va_list args)
{
    memblock::clear();
    return insertv (begin(), fmt, args);
}

int string::assignf (const char* fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    auto rv = assignv (fmt, args);
    va_end (args);
    return rv;
}

string::iterator string::replace (const_iterator f, const_iterator l, value_type c, size_type n)
{
    auto dsz = difference_type(n) - (l-f);
    if (dsz > 0)
	f = memblock::insert (f, dsz);
    else
	f = memblock::erase (f, -dsz);
    auto iwp = p2i (f);
    memset (iwp, c, n);
    return iwp;
}

auto string::rfind (const_pointer s, const_iterator fi) const -> const_iterator
{
    const_iterator r = nullptr;
    if (!fi)
	return r;
    fi -= zstr::length(s);
    for (auto h = begin(); (h = strstr(h,s)) && h < fi; ++h)
	r = h;
    return r;
}

} // namespace cwiclo
