// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "algo.h"
#include "sysutil.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/un.h>
#if __has_include(<arpa/inet.h>) && !defined(NDEBUG)
    #include <arpa/inet.h>
#endif
#include <time.h>

//----------------------------------------------------------------------
namespace cwiclo {

zstr::index_type zstr::nstrs (const_pointer p, difference_type n) // static
{
    index_type ns = 0;
    for (auto i = in(p,n); i; ++i) ++ns;
    return ns;
}

zstr::const_pointer zstr::at (index_type i, const_pointer p, difference_type n) // static
{
    assert (i < nstrs(p,n) && "zstr index out of range");
    return *(in(p,n) += i);
}

zstr::index_type zstr::index (const_pointer k, const_pointer p, difference_type n, index_type nf) // static
{
    difference_type ksz = strlen(k)+1;
    for (index_type i = 0; n; ++i) {
	auto np = next (p,n);
	if (np-p == ksz && compare (p, k, ksz))
	    return i;
	p = np;
    }
    return nf;
}

//----------------------------------------------------------------------

} // namespace cwiclo
using namespace cwiclo;

//----------------------------------------------------------------------

uint64_t now_milliseconds (void)
{
    struct timespec t;
    if (0 > clock_gettime (CLOCK_REALTIME, &t))
	return 0;
    return t.tv_nsec/1000000 + t.tv_sec*1000;
}

#ifdef __linux__ // closefrom is in libc on bsd
void closefrom (int fd)
{
    for (int tofd = getdtablesize(); fd < tofd; ++fd)
	close (fd);
}
#endif

int make_fd_nonblocking (int fd)
{
    auto f = fcntl (fd, F_GETFL);
    return f < 0 ? f : fcntl (fd, F_SETFL, f| O_NONBLOCK);
}

int make_fd_blocking (int fd)
{
    auto f = fcntl (fd, F_GETFL);
    return f < 0 ? f : fcntl (fd, F_SETFL, f&~(O_NONBLOCK));
}

#ifndef UC_VERSION

int mkpath (const char* path, mode_t mode)
{
    char pbuf [PATH_MAX];
    auto n = size_t(snprintf (ARRAY_BLOCK(pbuf), "%s", path));
    if (n > size(pbuf))
	return -1;
    for (auto f = begin(pbuf), l = f+n; f < l; ++f) {
	f = find (f, l, '/');
	if (!f)
	    f = l;
	*f = 0;
	if (0 > mkdir (pbuf, mode) && errno != EEXIST)
	    return -1;
	*f = '/';
    }
    return 0;
}

int rmpath (const char* path)
{
    char pbuf [PATH_MAX];
    if (size(pbuf) < size_t(snprintf (ARRAY_BLOCK(pbuf), "%s", path)))
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

unsigned sd_listen_fds (void)
{
    const char* e = getenv("LISTEN_PID");
    if (!e || getpid() != pid_t(atoi(e)))
	return 0;
    e = getenv("LISTEN_FDS");
    return e ? atoi(e) : 0;
}

int sd_listen_fd_by_name (const char* name)
{
    const char* na = getenv("LISTEN_FDNAMES");
    if (!na)
	return -1;
    auto namelen = strlen(name);
    for (auto fdi = 0u; *na; ++fdi) {
	const char *ee = strchr(na,':');
	if (!ee)
	    ee = na+strlen(na);
	if (equal_n (name, namelen, na, ee-na))
	    return fdi < sd_listen_fds() ? SD_LISTEN_FDS_START+fdi : -1;
	if (!*ee)
	    break;
	na = ee+1;
    }
    return -1;
}

#endif // UC_VERSION

#ifndef NDEBUG
const char* debug_socket_name (const struct sockaddr* addr)
{
    static char s_snbuf [256];
    if (addr->sa_family == PF_LOCAL) {
	auto a = pointer_cast<sockaddr_un>(addr);
	snprintf (ARRAY_BLOCK(s_snbuf), "%c%s", a->sun_path[0] ? a->sun_path[0] : '@', &a->sun_path[1]);
    } else if (addr->sa_family == PF_INET) {
	auto a = pointer_cast<sockaddr_in>(addr);
	char addrbuf [64];
	snprintf (ARRAY_BLOCK(s_snbuf), "%s:%hu", inet_ntop (PF_INET, &a->sin_addr, ARRAY_BLOCK(addrbuf)), a->sin_port);
    } else if (addr->sa_family == PF_INET6) {
	auto a = pointer_cast<sockaddr_in6>(addr);
	char addrbuf [64];
	snprintf (ARRAY_BLOCK(s_snbuf), "%s:%hu", inet_ntop (PF_INET6, &a->sin6_addr, ARRAY_BLOCK(addrbuf)), a->sin6_port);
    } else
	snprintf (ARRAY_BLOCK(s_snbuf), "SF%u", addr->sa_family);
    return s_snbuf;
}
#endif

int create_sockaddr_un (struct sockaddr_un* addr, const char* fmt, const char* path, const char* sockname)
{
    addr->sun_family = PF_LOCAL;
    auto psockpath = begin (addr->sun_path);
    auto sockpathsz = size (addr->sun_path);
    if (sockname[0] == '@') {
	++sockname;
	#ifdef __linux__
	    --sockpathsz;
	    *psockpath++ = 0;
	#endif
    }
    unsigned pathlen = snprintf (psockpath, sockpathsz, fmt, path, sockname);
    if (sockpathsz <= pathlen) {
	errno = ENAMETOOLONG;
	return -1;
    }
    return psockpath+pathlen-pointer_cast<char>(addr);
}

int socket_enable_credentials_passing (int sockfd, bool enable)
{
    int sov = enable;
    return setsockopt (sockfd, SOL_SOCKET, SO_PASSCRED, &sov, sizeof(sov));
}
