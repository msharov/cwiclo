// This file is part of the casycom project
//
// Copyright (c) 2015 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "xcom.h"
#include "app.h"
#include <sys/un.h>

namespace cwiclo {

auto IExtern::connect (const sockaddr* addr, socklen_t addrlen) const -> fd_t
{
    DEBUG_PRINTF ("[X] Connecting to socket %s\n", debug_socket_name(addr));
    auto fd = socket (addr->sa_family, SOCK_STREAM| SOCK_NONBLOCK| SOCK_CLOEXEC, 0);
    if (fd < 0)
	return fd;
    if (0 > ::connect (fd, addr, addrlen) && errno != EINPROGRESS && errno != EINTR) {
	DEBUG_PRINTF ("[E] connect failed: %s\n", strerror(errno));
	::close (fd);
	return fd = -1;
    }
    open (fd);
    return fd;
}

auto IExtern::connect_local (const char* sockname) const -> fd_t
{
    fd_t sfd = -1;
    sockaddr_un addr;
    do {
	auto addrlen = create_sockaddr_un (&addr, socket_path_from_name(sockname).c_str());
	if (addrlen < 0)
	    return sfd;
	sfd = connect (pointer_cast<sockaddr>(&addr), addrlen);
	if (sfd < 0 && sockname[0] == '@') {
	    ++sockname;
	    continue;	// If abstract socket connection fails, try the file socket
	}
    } while (false);
    return sfd;
}

auto IExtern::launch_pipe (const char* exe, const char* arg) const -> fd_t
{
    // Create socket pipe, will be connected to stdin in server
    enum { socket_ClientSide, socket_ServerSide, socket_N };
    fd_t socks [socket_N];
    if (0 > socketpair (PF_LOCAL, SOCK_STREAM| SOCK_NONBLOCK, 0, socks))
	return -1;

    if (auto fr = fork(); fr < 0) {
	::close (socks[socket_ClientSide]);
	::close (socks[socket_ServerSide]);
	return -1;
    } else if (!fr) {
	// Server side

	// setup socket-activation-style fd passing
	char pids [16];
	setenv ("LISTEN_PID", uint_to_text(getpid(), pids), true);
	setenv ("LISTEN_FDS", "1", true);
	setenv ("LISTEN_FDNAMES", "connection", true);

	const fd_t fd = SD_LISTEN_FDS_START+0;
	dup2 (socks[socket_ServerSide], fd);
	closefrom (fd+1);

	execlp (exe, exe, arg, nullptr);

	// If exec failed, log the error and exit
	Msger::error ("failed to launch pipe to '%s': %s", exe, strerror(errno));
	exit (EXIT_FAILURE);
    }
    // Client side
    ::close (socks[socket_ServerSide]);
    open (socks[socket_ClientSide]);
    return socks[socket_ClientSide];
}

} // namespace cwiclo
