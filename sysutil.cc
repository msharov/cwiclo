// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "algo.h"
#include "sysutil.h"
#include "appl.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/un.h>
#if __has_include(<arpa/inet.h>)
    #include <arpa/inet.h>
#endif
#include <time.h>

//{{{ File descriptor and path utilities -------------------------------
namespace cwiclo {

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

#endif

string substitute_environment_vars (const string_view& s)
{
    string r = s;
    string::iterator vars = nullptr;
    bool inquote = false;
    char lc = 0;
    for (auto i = r.begin();; ++i) {
	if (vars && (i >= r.end() || !(*i == '_' || (*i >= 'a' && *i <= 'z') || (*i >= 'A' && *i <= 'Z')))) {
	    auto termc = exchange (*i, 0);
	    const char* ev = getenv (1+vars);
	    if (!ev)
		ev = "";
	    auto evn = zstr::length (ev);
	    i = evn + r.replace (vars, i, ev, evn);
	    *i = termc;
	    vars = nullptr;
	}
	if (i >= r.end())
	    return r;
	if (*i == '\'' && lc != '\\')
	    inquote = !inquote;
	else if (!inquote && *i == '$' && lc != '\\')
	    vars = i;
	lc = *i;
    }
}

//}}}-------------------------------------------------------------------
//{{{ Miscellaneous

// Randomly initializes the random number generator
void srandrand (void)
{
    struct timespec now;
    clock_gettime (CLOCK_REALTIME, &now);
    uint32_t seed = now.tv_sec;
    seed ^= getppid();
    seed = bit_ror (seed, 16);
    seed ^= getpid();
    seed ^= now.tv_nsec;
    srand (seed);
}

//}}}-------------------------------------------------------------------
//{{{ Socket utilities

int socket_enable_credentials_passing (int sockfd, bool enable)
{
    int sov = enable;
    return setsockopt (sockfd, SOL_SOCKET, SO_PASSCRED, &sov, sizeof(sov));
}

#ifndef UC_VERSION

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

int create_sockaddr_un (struct sockaddr_un* addr, const char* sockname)
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
    auto pathlen = zstr::length (sockname);
    if (pathlen >= sockpathsz) {
	errno = ENAMETOOLONG;
	return -1;
    }
    copy_n (sockname, pathlen+1, psockpath);
    return psockpath+pathlen-pointer_cast<char>(addr);
}

int connect_to_socket (const sockaddr* addr, socklen_t addrlen)
{
    debug_printf ("Connecting to socket %s\n", debug_socket_name(addr));
    auto fd = socket (addr->sa_family, SOCK_STREAM| SOCK_NONBLOCK| SOCK_CLOEXEC, 0);
    if (fd < 0)
	return fd;
    if (0 > connect (fd, addr, addrlen) && errno != EINPROGRESS && errno != EINTR) {
	debug_printf ("[E] connect failed: %s\n", strerror(errno));
	close (exchange (fd, -1));
    }
    return fd;
}

int connect_to_local_socket (const char* sockname)
{
    int sfd = -1;
    sockaddr_un addr;
    do {
	auto addrlen = create_sockaddr_un (&addr, socket_path_from_name(sockname).c_str());
	if (addrlen < 0)
	    return sfd;
	sfd = connect_to_socket (pointer_cast<sockaddr>(&addr), addrlen);
	if (sfd < 0 && sockname[0] == '@') {
	    ++sockname;
	    continue;	// If abstract socket connection fails, try the file socket
	}
    } while (false);
    return sfd;
}

// Abstract sockets live outside the filesystem,
// so the server side must do manual permissions checking.
//
uid_t uid_filter_for_local_socket (int fd)
{
    sockaddr_storage ss;
    socklen_t l = sizeof(ss);
    if (getsockname (fd, pointer_cast<sockaddr>(&ss), &l) < 0)
	return 0;
    if (ss.ss_family != PF_LOCAL)
	return 0;

    sockaddr_un* sun = pointer_cast<sockaddr_un>(&ss);
    if (!sun->sun_path[0]) {
	// Find existing path component and use its uid as filter
	auto pdirn = &sun->sun_path[1];
	debug_printf ("Using abstract socket %s\n", pdirn);
	auto pdire = pointer_cast<char>(&ss)+l;
	while (--pdire > pdirn) {
	    struct stat st;
	    if (exchange (*pdire, 0) == '/' && 0 == stat (pdirn, &st) && S_ISDIR(st.st_mode)) {
		debug_printf ("Setting uid filter to %d, owner of %s\n", st.st_uid, pdirn);
		return st.st_uid;
	    }
	}
    }
    return 0;
}

int launch_pipe (const char* exe, const char* arg)
{
    // Create socket pipe, will be connected to stdin in server
    enum { socket_ClientSide, socket_ServerSide, socket_N };
    int socks [socket_N];
    if (0 > socketpair (PF_LOCAL, SOCK_STREAM| SOCK_NONBLOCK, 0, socks))
	return -1;

    if (auto fr = fork(); fr < 0) {
	close (exchange (socks[socket_ClientSide], -1));
	close (socks[socket_ServerSide]);
	return socks[socket_ClientSide];
    } else if (!fr) {
	// Server side

	// setup socket-activation-style fd passing
	char pids [16];
	setenv ("LISTEN_PID", uint_to_text(getpid(), pids), true);
	setenv ("LISTEN_FDS", "1", true);
	setenv ("LISTEN_FDNAMES", "connection", true);

	const int fd = SD_LISTEN_FDS_START+0;
	dup2 (socks[socket_ServerSide], fd);
	closefrom (fd+1);

	execlp (exe, exe, arg, nullptr);

	// If exec failed, log the error and exit
	printf ("Failed to launch pipe to '%s': %s\n", exe, strerror(errno));
	exit (EXIT_FAILURE);
    }
    // Client side
    close (socks[socket_ServerSide]);
    return socks[socket_ClientSide];
}

string socket_path_from_name (const string_view& name)
{
    string r = substitute_environment_vars (name);
    auto i = r.begin();

    // Abstract sockets
    if (*i == '@') {
	#ifdef __linux__
	    ++i;
	#else // not supported on BSD; fallback to path
	    i = r.erase (i);
	#endif
    }

    // Absolute path name
    char pathtype = *i;
    if (pathtype == '/')
	return r;

    // System socket in /run
    #ifdef __linux__
	auto rundir = "/run";
    #else // unless on BSD, where there is no /run
	auto rundir = "/var/run";
    #endif

    // User socket in standard location, from $XDG_RUNTIME_DIR
    if (pathtype == '~') {
	if (auto urdir = getenv ("XDG_RUNTIME_DIR"); urdir)
	    rundir = urdir;
	*i = '/';
    } else
	i = r.insert (i, '/');
    r.insert (i, rundir);
    return r;
}

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

//}}}-------------------------------------------------------------------
//{{{ chrono

namespace chrono {

auto system_clock::now (void) -> rep
{
    struct timespec t;
    if (0 > clock_gettime (CLOCK_REALTIME, &t))
	return 0;
    return t.tv_nsec/1000000 + t.tv_sec*1000;
}

} // namespace chrono
} // namespace cwiclo
//}}}-------------------------------------------------------------------
