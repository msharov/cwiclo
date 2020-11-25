// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "memory.h"
#include <sys/socket.h>

namespace cwiclo {
namespace {

// Read ntr bytes from fd, accounting for partial reads and EINTR
inline static int complete_read (int fd, void* p, size_t ntr)
{
    int nr = 0;
    while (ntr) {
	auto r = read (fd, p, ntr);
	if (r <= 0) {
	    if (errno == EINTR)
		continue;
	    return -1;
	}
	ntr -= r;
	nr += r;
	advance (p, r);
    }
    return nr;
}

// Write ntw bytes to fd, accounting for partial writes and EINTR
inline static int complete_write (int fd, const void* p, size_t ntw)
{
    int nw = 0;
    while (ntw) {
	auto r = write (fd, p, ntw);
	if (r <= 0) {
	    if (errno == EINTR)
		continue;
	    return -1;
	}
	ntw -= r;
	nw += r;
	advance (p, r);
    }
    return nw;
}

// Number printing without printf, which can undesirably load locales
template <unsigned N>
static constexpr char* uint_to_text (unsigned n, char (&a)[N])
{
    auto t = end(a);
    *--t = 0;
    for (; n; n /= 10)
	*--t = '0'+(n%10);
    return t;
}

} // namespace

//----------------------------------------------------------------------
// Socket credentials passing API for Linux and BSD

struct SocketCredentials {
    pid_t	pid;
    uid_t	uid;
    #ifdef SCM_CREDS
	uid_t	euid; // BSD-only field, do not use
    #endif
    gid_t	gid;
};

#ifdef SCM_CREDS // BSD interface
    enum { SCM_CREDENTIALS = SCM_CREDS };
    enum { SO_PASSCRED = LOCAL_PEERCRED };
    using ucred = cmsgcred;
#elif !defined(SCM_CREDENTIALS)
    #error "this platform does not support socket credentials passing"
#endif

//----------------------------------------------------------------------

} // namespace cwiclo
extern "C" {

uint64_t now_milliseconds (void);

#ifdef __linux__
void closefrom (int fd);
#endif

int make_fd_nonblocking (int fd);
int make_fd_blocking (int fd);

#ifndef UC_VERSION
int mkpath (const char* path, mode_t mode) NONNULL();
int rmpath (const char* path) NONNULL();

enum { SD_LISTEN_FDS_START = STDERR_FILENO+1 };
unsigned sd_listen_fds (void);
int sd_listen_fd_by_name (const char* name);
#endif

#ifndef NDEBUG
const char* debug_socket_name (const struct sockaddr* addr);
#endif

int create_sockaddr_un (struct sockaddr_un* addr, const char* fmt, const char* path, const char* sockname);

inline int create_sockaddr_local (struct sockaddr_un* addr, const char* sockname)
    { return create_sockaddr_un (addr, "%s%s", "", sockname); }
inline int create_sockaddr_system_local (struct sockaddr_un* addr, const char* sockname)
    { return create_sockaddr_un (addr, "%s/%s", "/run", sockname); }
inline int create_sockaddr_user_local (struct sockaddr_un* addr, const char* sockname)
{
    auto rundir = getenv ("XDG_RUNTIME_DIR");
    return create_sockaddr_un (addr, "%s/%s", rundir ? rundir : "/tmp", sockname);
}

int socket_enable_credentials_passing (int sockfd, bool enable);

} // extern "C"
