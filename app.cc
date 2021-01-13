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
    if (auto e = getenv("LISTEN_FDS"); !e || 64 < (nfds = atoi(e))) {
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
    auto sockpath = socket_path_from_name (sockname);
    if (sockpath.empty())
	return;	// some exported interfaces may not have a specific associated socket
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

iid_t App::listed_interface_by_name (const iid_t* il, const char* is, size_t islen) // static
{
    for (auto i = il; *i; ++i)
	if (equal_n (*i, interface_name_size(*i), is, islen))
	    return *i;
    return nullptr;
}

iid_t App::extern_interface_by_name (const char* is, size_t islen) const
{
    auto iid = listed_interface_by_name (imports(), is, islen);
    if (!iid)
	iid = listed_interface_by_name (exports(), is, islen);
    return iid;
}

Extern* App::extern_by_id (mrid_t eid) const
{
    for (auto& is : _isock)
	if (is.dest() == eid)
	    return pointer_cast<Extern>(msger_by_id (is.dest()));
    return nullptr;
}

Extern* App::create_extern_dest_for (iid_t iid)
{
    // Verify that it is on the imports list
    if (!extern_interface_by_name (iid, interface_name_size(iid)))
	return nullptr;

    // Check if an existing Extern object imports it
    Extern* ee = nullptr;
    for (auto& is : _isock) {
	auto e = pointer_cast<Extern>(msger_by_id (is.dest()));
	auto& info = e->info();
	if (info.imported.empty() && !info.creds.pid)
	    ee = e;	// ee has not established the connection yet
	else if (info.is_importing (iid))
	    return e;
    }

    // Check if an Extern object has recently been launched.
    // It may or may not be importing iid, but giving messages to it
    // prevents launching multiple server processes for the same iid.
    // When the connection is established, Extern will check the imports
    // list and will return incompatible messages.
    if (ee)
	return ee;

    // No Extern objects supporting the interface exist. Try to create one.
    auto isockname = interface_socket_name (iid);
    auto iprogname = interface_program_name (iid);
    if (!isockname[0] && !iprogname[0])
	return nullptr;	// no connection information specified

    auto& extp = _isock.emplace_back (msger_id());
    auto sfd = -1;

    // First try to connect to the interface-specified socket name.
    if (isockname[0])
	sfd = extp.connect_local (isockname);

    // Then try to launch the default server program
    if (sfd < 0 && iprogname[0])
	sfd = extp.launch_pipe (iprogname);

    // If both failed, there is nothing more that can be done
    if (sfd < 0) {
	_isock.pop_back();
	return nullptr;
    }

    // Otherwise, the Extern object will have been created at this point
    return pointer_cast<Extern>(msger_by_id (extp.dest()));
}

} // namespace cwiclo
