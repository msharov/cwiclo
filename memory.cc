// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "memory.h"
#include <alloca.h>
#include <sys/stat.h>
#if __has_include(<execinfo.h>)
    #include <execinfo.h>
#endif

extern "C" void* _realloc (void* p, size_t n) noexcept
{
    p = realloc (p, n);
    if (!p)
	abort();
    return p;
}

extern "C" void* _alloc (size_t n) noexcept
{
    auto p = _realloc (nullptr, n);
    #ifndef NDEBUG
	memset (p, 0xcd, n);
    #endif
    return p;
}

extern "C" void _free (void* p) noexcept
    { free(p); }

//----------------------------------------------------------------------

void* operator new (size_t n)	WEAKALIAS("_alloc");
void* operator new[] (size_t n)	WEAKALIAS("_alloc");

void  operator delete (void* p) noexcept	WEAKALIAS("_free");
void  operator delete[] (void* p) noexcept	WEAKALIAS("_free");
void  operator delete (void* p, size_t n) noexcept	WEAKALIAS("_free");
void  operator delete[] (void* p, size_t n) noexcept	WEAKALIAS("_free");

//----------------------------------------------------------------------

namespace cwiclo {

extern "C" void brotate (void* vf, void* vm, void* vl) noexcept
{
    auto f = (char*) vf, m = (char*) vm, l = (char*) vl;
    const auto lsz (m-f), hsz (l-m), hm (min (lsz, hsz));
    if (!hm)
	return;
    auto t = alloca (hm);
    if (hsz < lsz) {
	memcpy (t, m, hsz);
	memmove (f+hsz, f, lsz);
	memcpy (f, t, hsz);
    } else {
	memcpy (t, f, lsz);
	memmove (f, m, hsz);
	memcpy (l-lsz, t, lsz);
    }
}

extern "C" void print_backtrace (void) noexcept
{
#if __has_include(<execinfo.h>)
    void* frames[32];
    auto nf = backtrace (ArrayBlock(frames));
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
	    if constexpr (sizeof(void*) <= 8)
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
static inline char _num_to_digit (uint8_t b)
{
    char d = (b & 0xF) + '0';
    return d <= '9' ? d : d+('A'-'0'-10);
}
static inline bool _printable (char c)
{
    return c >= 32 && c < 127;
}
extern "C" void hexdump (const void* vp, size_t n) noexcept
{
    auto p = (const uint8_t*) vp;
    char line[65]; line[64] = 0;
    for (size_t i = 0; i < n; i += 16) {
	memset (line, ' ', sizeof(line)-1);
	for (size_t h = 0; h < 16; ++h) {
	    if (i+h < n) {
		uint8_t b = p[i+h];
		line[h*3] = _num_to_digit(b>>4);
		line[h*3+1] = _num_to_digit(b);
		line[h+3*16] = _printable(b) ? b : '.';
	    }
	}
	puts (line);
    }
}

//----------------------------------------------------------------------

const char* executable_in_path (const char* efn, char* exe, size_t exesz) noexcept
{
    if (efn[0] == '/' || (efn[0] == '.' && (efn[1] == '/' || efn[1] == '.'))) {
	if (0 != access (efn, X_OK))
	    return NULL;
	return efn;
    }

    const char* penv = getenv("PATH");
    if (!penv)
	penv = "/bin:/usr/bin:.";
    char path [PATH_MAX];
    snprintf (ArrayBlock(path), "%s/%s"+3, penv);

    for (char *pf = path, *pl = pf; *pf; pf = pl) {
	while (*pl && *pl != ':') ++pl;
	*pl++ = 0;
	snprintf (exe, exesz, "%s/%s", pf, efn);
	if (0 == access (exe, X_OK))
	    return exe;
    }
    return NULL;
}

unsigned sd_listen_fds (void) noexcept
{
    const char* e = getenv("LISTEN_PID");
    if (!e || getpid() != (pid_t) strtoul(e, NULL, 10))
	return 0;
    e = getenv("LISTEN_FDS");
    return e ? strtoul (e, NULL, 10) : 0;
}

int mkpath (const char* path, mode_t mode) noexcept
{
    char pbuf [PATH_MAX], *pbe = pbuf;
    do {
	if (*path == '/' || !*path) {
	    *pbe = '\0';
	    if (pbuf[0] && 0 > mkdir (pbuf, mode) && errno != EEXIST)
		return -1;
	}
	*pbe++ = *path;
    } while (*path++);
    return 0;
}

int rmpath (const char* path) noexcept
{
    char pbuf [PATH_MAX];
    for (char* pend = stpcpy (pbuf, path)-1;; *pend = 0) {
	if (0 > rmdir(pbuf))
	    return (errno == ENOTEMPTY || errno == EACCES) ? 0 : -1;
	do {
	    if (pend <= pbuf)
		return 0;
	} while (*--pend != '/');
    }
    return 0;
}
#endif // UC_VERSION

} // namespace cwiclo
