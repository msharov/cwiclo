// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "memory.h"
#include <alloca.h>
#if __has_include(<execinfo.h>)
    #include <execinfo.h>
#endif

namespace std {

void terminate (void) noexcept { abort(); }

} // namespace std
namespace __cxxabiv1 {
extern "C" {

#ifndef NDEBUG
#define TERMINATE_ALIAS(name)	void name (void) noexcept { assert (!#name); std::terminate(); }
#else
#define TERMINATE_ALIAS(name)	void name (void) noexcept WEAKALIAS("_ZSt9terminatev");
#endif
TERMINATE_ALIAS (__cxa_call_unexpected)
TERMINATE_ALIAS (__cxa_pure_virtual)
TERMINATE_ALIAS (__cxa_deleted_virtual)
TERMINATE_ALIAS (__cxa_bad_cast)
TERMINATE_ALIAS (__cxa_bad_typeid)
TERMINATE_ALIAS (__cxa_throw_bad_array_new_length)
TERMINATE_ALIAS (__cxa_throw_bad_array_length)
#undef TERMINATE_ALIAS

int __cxa_guard_acquire (__guard* g) noexcept { return !g->test_and_set(); }
void __cxa_guard_release (__guard*) noexcept {}
void __cxa_guard_abort (__guard* g) noexcept { g->clear(); }

} // extern "C"
} // namespace __cxxabiv1

//----------------------------------------------------------------------

void* _realloc (void* p, size_t n) noexcept
{
    p = realloc (p, n);
    if (!p)
	abort();
    return p;
}

void* _alloc (size_t n) noexcept
{
    auto p = _realloc (nullptr, n);
    #ifndef NDEBUG
	cwiclo::fill_n (static_cast<char*>(p), n, '\xcd');
    #endif
    return p;
}

namespace cwiclo {

void brotate (void* vf, void* vm, void* vl) noexcept
{
    auto f = (char*) vf, m = (char*) vm, l = (char*) vl;
    const auto lsz (m-f), hsz (l-m), hm (min (lsz, hsz));
    if (!hm)
	return;
    auto t = alloca (hm);
    if (hsz < lsz) {
	copy_n (m, hsz, t);
	copy_backward_n (f, lsz, f+hsz);
	copy_n (t, hsz, f);
    } else {
	copy_n (f, lsz, t);
	copy_backward_n (m, hsz, f);
	copy_n (t, lsz, l-lsz);
    }
}

void print_backtrace (void) noexcept
{
#if __has_include(<execinfo.h>)
    void* frames[32];
    auto nf = backtrace (ARRAY_BLOCK(frames));
    if (nf <= 1)
	return;
    auto syms = backtrace_symbols (frames, nf);
    if (!syms)
	return;
    for (auto f = 1; f < nf; ++f) {
	// Symbol names are formatted thus: "file(function+0x42) [0xAddress]"
	// Parse out the function name
	auto fnstart = strchr (syms[f], '(');
	if (!fnstart)
	    fnstart = syms[f];
	else
	    ++fnstart;
	auto fnend = strchr (fnstart, '+');
	if (!fnend) {
	    fnend = strchr (fnstart, ')');
	    if (!fnend)
		fnend = fnstart + strlen(fnstart);
	}
	auto addrstart = strchr (fnend, '[');
	if (addrstart) {
	    auto addr = strtoul (++addrstart, nullptr, 0);
	    if constexpr (sizeof(void*) < 8)
		printf ("%8lx\t", addr);
	    else
		printf ("%16lx\t", addr);
	}
	fwrite (fnstart, 1, fnend-fnstart, stdout);
	fputc ('\n', stdout);
    }
    free (syms);
    fflush (stdout);
#endif
}

#ifndef UC_VERSION
void hexdump (const void* vp, size_t n) noexcept
{
    //{{{ CP437 UTF8 table
    static const char c_437table[256][4] = {
	   " ", "\u263A", "\u263B", "\u2665", "\u2666", "\u2663", "\u2660", "\u2022",
	"\u25D8", "\u25CB", "\u25D9", "\u2642", "\u2640", "\u266A", "\u266B", "\u263C",
	"\u25BA", "\u25C4", "\u2195", "\u203C", "\u00B6", "\u00A7", "\u25AC", "\u21A8",
	"\u2191", "\u2193", "\u2192", "\u2190", "\u221F", "\u2194", "\u25B2", "\u25BC",
	   " ", "!", "\"", "#", "$", "%", "&", "\'", "(", ")", "*", "+", ",", "-", ".", "/",
	   "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",
	   "@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
	   "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\", "]", "^", "_",
	   "`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
	   "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~", "\u2302",
	"\u00C7", "\u00FC", "\u00E9", "\u00E2", "\u00E4", "\u00E0", "\u00E5", "\u00E7",
	"\u00EA", "\u00EB", "\u00E8", "\u00EF", "\u00EE", "\u00EC", "\u00C4", "\u00C5",
	"\u00C9", "\u00E6", "\u00C6", "\u00F4", "\u00F6", "\u00F2", "\u00FB", "\u00F9",
	"\u00FF", "\u00D6", "\u00DC", "\u00A2", "\u00A3", "\u00A5", "\u20A7", "\u0192",
	"\u00E1", "\u00ED", "\u00F3", "\u00FA", "\u00F1", "\u00D1", "\u00AA", "\u00BA",
	"\u00BF", "\u2310", "\u00AC", "\u00BD", "\u00BC", "\u00A1", "\u00AB", "\u00BB",
	"\u2591", "\u2592", "\u2593", "\u2502", "\u2524", "\u2561", "\u2562", "\u2556",
	"\u2555", "\u2563", "\u2551", "\u2557", "\u255D", "\u255C", "\u255B", "\u2510",
	"\u2514", "\u2534", "\u252C", "\u251C", "\u2500", "\u253C", "\u255E", "\u255F",
	"\u255A", "\u2554", "\u2569", "\u2566", "\u2560", "\u2550", "\u256C", "\u2567",
	"\u2568", "\u2564", "\u2565", "\u2559", "\u2558", "\u2552", "\u2553", "\u256B",
	"\u256A", "\u2518", "\u250C", "\u2588", "\u2584", "\u258C", "\u2590", "\u2580",
	"\u03B1", "\u00DF", "\u0393", "\u03C0", "\u03A3", "\u03C3", "\u00B5", "\u03C4",
	"\u03A6", "\u0398", "\u03A9", "\u03B4", "\u221E", "\u03C6", "\u03B5", "\u2229",
	"\u2261", "\u00B1", "\u2265", "\u2264", "\u2320", "\u2321", "\u00F7", "\u2248",
	"\u00B0", "\u2219", "\u00B7", "\u221A", "\u207F", "\u00B2", "\u25A0", "\u00A0"
    };
    //}}}
    auto p = static_cast<const uint8_t*>(vp);
    for (auto i = 0u; i < n; i += 16) {
	for (auto j = 0u; j < 16; ++j) {
	    if (i+j < n)
		printf ("%02x ", p[i+j]);
	    else
		printf ("   ");
	}
	for (auto j = 0u; j < 16 && i+j < n; ++j)
	    printf ("%s", c_437table[p[i+j]]);
	printf ("\n");
    }
}
#endif // UC_VERSION

} // namespace cwiclo
