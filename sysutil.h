// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "string.h"
#include <sys/socket.h>

//{{{ Socket prototypes ------------------------------------------------

struct sockaddr_un;

//}}}-------------------------------------------------------------------
//{{{ EINTR-aware read and write

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

//}}}-------------------------------------------------------------------
//{{{ uint_to_text

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

//}}}-------------------------------------------------------------------
//{{{ Socket credentials passing API for Linux and BSD

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

int socket_enable_credentials_passing (int sockfd, bool enable);

//}}}-------------------------------------------------------------------
//{{{ Other socket utilities

string socket_path_from_name (const string_view& name);
int create_sockaddr_un (struct sockaddr_un* addr, const char* sockname);
int connect_to_socket (const struct sockaddr* addr, socklen_t addrlen);
int connect_to_local_socket (const char* path);
int launch_pipe (const char* exe, const char* arg = nullptr);
const char* debug_socket_name (const struct sockaddr* addr);

#ifndef UC_VERSION
enum { SD_LISTEN_FDS_START = STDERR_FILENO+1 };
unsigned sd_listen_fds (void);
int sd_listen_fd_by_name (const char* name);
#endif

//}}}-------------------------------------------------------------------
//{{{ File descriptor and path utilities

#ifdef __linux__
void closefrom (int fd);
#endif

int make_fd_nonblocking (int fd);
int make_fd_blocking (int fd);

#ifndef UC_VERSION
int mkpath (const char* path, mode_t mode) NONNULL();
int rmpath (const char* path) NONNULL();
#endif

string substitute_environment_vars (const string_view& s);

//}}}-------------------------------------------------------------------
//{{{ chrono
namespace chrono {

class system_clock {
public:
    using rep = uint64_t;
    static constexpr const rep period = 1000000;
public:
    static rep	now (void);
};

} // namespace chrono
} // namespace cwiclo
//}}}-------------------------------------------------------------------
