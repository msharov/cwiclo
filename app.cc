// This file is part of the cwiclo project
//
// Copyright (c) 2021 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "app.h"
#include "xtern.h"
#include <sys/stat.h>
#include <sys/un.h>

namespace cwiclo {

IMPLEMENT_INTERFACES_D (App)

int App::run (void)
{
    if (!accept_socket_activation())
	for (auto ei = exports(); *ei; ++ei)
	    create_extern_socket (interface_socket_name (*ei));
    return AppL::run();
}

App::PListener::~PListener (void)
{
    if (_sockfd >= 0)
	close (exchange (_sockfd, -1));
    if (!_sockname.empty())
	unlink (_sockname.c_str());
    free_id();
}

bool App::accept_socket_activation (void)
{
    // First check if enabled
    if (auto e = getenv("LISTEN_PID"); !e || getpid() != pid_t(atoi(e)))
	return false;

    // Not having LISTEN_FDS or having too many are errors
    unsigned nfds = 0;
    if (auto e = getenv("LISTEN_FDS"); !e || 32 < (nfds = atoi(e))) {
	error ("invalid LISTEN_FDS");
	return true;
    }

    // Socket names can specify an accepted connection
    auto sockname = getenv ("LISTEN_FDNAMES");
    // They are, however, optional, so sockname can be null

    for (auto i = 0u; i < nfds; ++i) {
	bool accepted = false;
	if (sockname) {
	    if (0 == strncmp (sockname, ARRAY_BLOCK("connection")-1))
		accepted = true;
	    sockname = strchr (sockname, ',');
	    if (sockname)
		++sockname;
	}
	fd_t fd = SD_LISTEN_FDS_START+i;
	// Passed in sockets are either already accepted, when named "connection"
	if (accepted)
	    add_extern_connection (fd);
	else // or they are to be listened on
	    add_extern_socket (fd);
    }
    return true;
}

void App::create_extern_socket (const char* sockname)
{
    auto sockpath = socket_path_from_name(sockname);
    sockaddr_un addr;
    auto addrlen = create_sockaddr_un (&addr, sockpath.c_str());
    if (addrlen < 0)
	return error ("socket name '%s' is too long", sockpath.c_str());
    DEBUG_PRINTF ("[X] Creating server socket %s\n", debug_socket_name(pointer_cast<sockaddr>(&addr)));
    auto fd = socket (PF_LOCAL, SOCK_STREAM| SOCK_NONBLOCK| SOCK_CLOEXEC, 0);
    if (fd < 0)
	return error_libc ("socket");
    if (0 > ::bind (fd, pointer_cast<sockaddr>(&addr), addrlen) && errno != EINPROGRESS) {
	::close (fd);
	return error ("%s bind: %s", sockpath.c_str(), strerror(errno));
    }
    if (0 > ::listen (fd, min (SOMAXCONN, 64))) {
	::close (fd);
	return error ("%s listen: %s", sockpath.c_str(), strerror(errno));
    }
    if (addr.sun_path[0])
	chmod (addr.sun_path, DEFFILEMODE);
    add_extern_socket (fd, addr.sun_path, WhenEmpty::Remain);
}

bool App::on_error (mrid_t eid, const string& errmsg)
{
    auto isock = find_if (_isock, [&](const auto& s){ return s.dest() == eid; });
    if (isock) {
	// Error in accepted socket.
	// Handle by logging the error and removing record in ObjectDestroyed.
	DEBUG_PRINTF ("[X] Client connection error: %s\n", errmsg.c_str());
	return true;
    }
    // All other errors are fatal.
    return AppL::on_error (eid, errmsg);
}

void App::on_msger_destroyed (mrid_t mid)
{
    DEBUG_PRINTF ("[X] Client connection %hu dropped\n", mid);
    remove_if (_isock, [&](const auto& e) { return e.dest() == mid; });
    if (_isock.empty() && !flag (f_ListenWhenEmpty))
	quit();
}

void App::Timer_timer (fd_t fd)
{
    auto esock = find_if (_esock, [&](const auto& s){ return s.sockfd() == fd; });
    if (!esock)
	return;
    for (int cfd; 0 <= (cfd = ::accept (esock->sockfd(), nullptr, nullptr));)
	add_extern_connection (cfd);
    if (errno == EAGAIN) {
	DEBUG_PRINTF ("[X] Resuming wait on fd %d\n", esock->sockfd());
	esock->wait_read();
    } else {
	DEBUG_PRINTF ("[X] Accept failed with error %s\n", strerror(errno));
	error_libc ("accept");
    }
}

void App::add_extern_socket (int fd, const char* sockname, WhenEmpty closeWhenEmpty)
{
    if (make_fd_nonblocking (fd))
	return error_libc ("make_fd_nonblocking");
    set_flag (f_ListenWhenEmpty, !bool(closeWhenEmpty));
    _esock.emplace_back (fd, sockname);
    Timer_timer (fd);
}

void App::add_extern_connection (int fd)
{
    DEBUG_PRINTF ("[X] Client connection accepted on fd %d\n", fd);
    if (_isock.empty())
	set_flag (f_Quitting, false);
    _isock.emplace_back (msger_id()).open (fd, exports());
}

} // namespace cwiclo
