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

//----------------------------------------------------------------------

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

namespace cwiclo {

extern "C" void brotate (void* vf, void* vm, void* vl) noexcept
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
extern "C" void hexdump (const void* vp, size_t n) noexcept
{
    auto p = reinterpret_cast<const uint8_t*>(vp);
    for (auto i = 0u; i < n; i += 16) {
	for (auto j = 0u; j < 16; ++j) {
	    if (i+j < n)
		printf ("%02x ", p[i+j]);
	    else
		printf ("   ");
	}
	for (auto j = 0u; j < 16 && i+j < n; ++j) {
	    auto c = p[i+j];
	    if (c >= ' ' && c <= '~')
		putchar (c);
	    else
		putchar (' ');
	}
	printf ("\n");
    }
}

//----------------------------------------------------------------------

const char* executable_in_path (const char* efn, char* exe, size_t exesz) noexcept
{
    if (efn[0] == '/' || (efn[0] == '.' && (efn[1] == '/' || efn[1] == '.'))) {
	if (0 != access (efn, X_OK))
	    return nullptr;
	return efn;
    }

    const char* penv = getenv("PATH");
    if (!penv)
	penv = "/bin:/usr/bin:.";
    char path [PATH_MAX];
    if (size(path) < size_t(snprintf (ArrayBlock(path), "%s/%s"+strlen("%s/"), penv)))
	return nullptr;

    for (char *pf = path, *pl = pf; *pf; pf = pl) {
	while (*pl && *pl != ':')
	    ++pl;
	*pl++ = 0;
	if (exesz < size_t(snprintf (exe, exesz, "%s/%s", pf, efn)))
	    return nullptr;
	if (0 == access (exe, X_OK))
	    return exe;
    }
    return nullptr;
}

unsigned sd_listen_fds (void) noexcept
{
    const char* e = getenv("LISTEN_PID");
    if (!e || getpid() != pid_t(atoi(e)))
	return 0;
    e = getenv("LISTEN_FDS");
    return e ? atoi(e) : 0;
}

int sd_listen_fd_by_name (const char* name) noexcept
{
    const char* na = getenv("LISTEN_FDNAMES");
    if (!na)
	return -1;
    auto namelen = strlen(name);
    for (auto fdi = 0u; *na; ++fdi) {
	const char *ee = strchr(na,':');
	if (!ee)
	    ee = na+strlen(na);
	if (size_t(ee-na) == namelen && 0 == memcmp (na, name, namelen))
	    return fdi < sd_listen_fds() ? SD_LISTEN_FDS_START+fdi : -1;
	if (!*ee)
	    break;
	na = ee+1;
    }
    return -1;
}

int mkpath (const char* path, mode_t mode) noexcept
{
    char pbuf [PATH_MAX];
    auto n = size_t(snprintf (ArrayBlock(pbuf), "%s", path));
    if (n > size(pbuf))
	return -1;
    for (auto f = begin(pbuf), l = f+n; f < l; ++f) {
	f = find (f, l, '/');
	*f = 0;
	if (0 > mkdir (pbuf, mode) && errno != EEXIST)
	    return -1;
	*f = '/';
    }
    return 0;
}

int rmpath (const char* path) noexcept
{
    char pbuf [PATH_MAX];
    if (size(pbuf) < size_t(snprintf (ArrayBlock(pbuf), "%s", path)))
	return -1;
    while (pbuf[0]) {
	if (0 > rmdir (pbuf))
	    return (errno == ENOTEMPTY || errno == EACCES) ? 0 : -1;
	auto pslash = strrchr (pbuf, '/');
	if (!pslash)
	    pslash = pbuf;
	*pslash = 0;
    }
    return 0;
}
#endif // UC_VERSION

} // namespace cwiclo
