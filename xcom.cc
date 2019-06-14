// This file is part of the casycom project
//
// Copyright (c) 2015 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "xcom.h"
#include <fcntl.h>
#include <syslog.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <paths.h>
#if __has_include(<arpa/inet.h>) && !defined(NDEBUG)
    #include <arpa/inet.h>
#endif

//{{{ COM --------------------------------------------------------------
namespace cwiclo {

DEFINE_INTERFACE (COM)

string PCOM::string_from_interface_list (const iid_t* elist) noexcept // static
{
    string elstr;
    if (elist && *elist) {
	for (auto e = elist; *e; ++e)
	    elstr.appendf ("%s,", *e);
	elstr.pop_back();
    }
    return elstr;
}

Msg PCOM::export_msg (const string& elstr) noexcept // static
{
    Msg msg (Msg::Link{}, PCOM::m_export(), stream_size_of(elstr), Msg::NoFdIncluded);
    msg.write() << elstr;
    return msg;
}

Msg PCOM::error_msg (const string& errmsg) noexcept // static
{
    Msg msg (Msg::Link{}, PCOM::m_error(), stream_size_of(errmsg), Msg::NoFdIncluded);
    msg.write() << errmsg;
    return msg;
}

//}}}-------------------------------------------------------------------
//{{{ PExtern

DEFINE_INTERFACE (Extern)

auto PExtern::connect (const sockaddr* addr, socklen_t addrlen) noexcept -> fd_t
{
    auto fd = socket (addr->sa_family, SOCK_STREAM| SOCK_NONBLOCK| SOCK_CLOEXEC, IPPROTO_IP);
    if (fd < 0)
	return fd;
    if (0 > ::connect (fd, addr, addrlen) && errno != EINPROGRESS && errno != EINTR) {
	DEBUG_PRINTF ("[E] Failed to connect to socket: %s\n", strerror(errno));
	::close (fd);
	return fd = -1;
    }
    open (fd);
    return fd;
}

/// Create local socket with given path
auto PExtern::connect_local (const char* path) noexcept -> fd_t
{
    sockaddr_un addr;
    addr.sun_family = PF_LOCAL;
    if (size(addr.sun_path) <= unsigned (snprintf (ARRAY_BLOCK(addr.sun_path), "%s", path))) {
	errno = ENAMETOOLONG;
	return -1;
    }
    DEBUG_PRINTF ("[X] connecting to socket %s\n", addr.sun_path);
    return connect (pointer_cast<sockaddr>(&addr), sizeof(addr));
}

/// Create local socket of the given name in the system standard location for such
auto PExtern::connect_system_local (const char* sockname) noexcept -> fd_t
{
    sockaddr_un addr;
    addr.sun_family = PF_LOCAL;
    if (size(addr.sun_path) <= unsigned (snprintf (ARRAY_BLOCK(addr.sun_path), _PATH_VARRUN "%s", sockname))) {
	errno = ENAMETOOLONG;
	return -1;
    }
    DEBUG_PRINTF ("[X] connecting to socket %s\n", addr.sun_path);
    return connect (pointer_cast<sockaddr>(&addr), sizeof(addr));
}

/// Create local socket of the given name in the user standard location for such
auto PExtern::connect_user_local (const char* sockname) noexcept -> fd_t
{
    sockaddr_un addr;
    addr.sun_family = PF_LOCAL;
    const char* runtimedir = getenv ("XDG_RUNTIME_DIR");
    if (!runtimedir)
	runtimedir = _PATH_TMP;
    if (size(addr.sun_path) <= unsigned (snprintf (ARRAY_BLOCK(addr.sun_path), "%s/%s", runtimedir, sockname))) {
	errno = ENAMETOOLONG;
	return -1;
    }
    DEBUG_PRINTF ("[X] connecting to socket %s\n", addr.sun_path);
    return connect (pointer_cast<sockaddr>(&addr), sizeof(addr));
}

auto PExtern::connect_ip4 (in_addr_t ip, in_port_t port) noexcept -> fd_t
{
    sockaddr_in addr = {};
    addr.sin_family = PF_INET;
    addr.sin_port = port;
    #ifdef UC_VERSION
	addr.sin_addr = ip;
    #else
	addr.sin_addr = { ip };
    #endif
#ifndef NDEBUG
    char addrbuf [64];
    #ifdef UC_VERSION
	DEBUG_PRINTF ("[X] connecting to socket %s:%hu\n", inet_intop(addr.sin_addr, ARRAY_BLOCK(addrbuf)), port);
    #else
	DEBUG_PRINTF ("[X] connecting to socket %s:%hu\n", inet_ntop(PF_INET, &addr.sin_addr, ARRAY_BLOCK(addrbuf)), port);
    #endif
#endif
    return connect (pointer_cast<sockaddr>(&addr), sizeof(addr));
}

/// Create local IPv4 socket at given port on the loopback interface
auto PExtern::connect_local_ip4 (in_port_t port) noexcept -> fd_t
    { return PExtern::connect_ip4 (INADDR_LOOPBACK, port); }

/// Create local IPv6 socket at given ip and port
auto PExtern::connect_ip6 (in6_addr ip, in_port_t port) noexcept -> fd_t
{
    sockaddr_in6 addr = {};
    addr.sin6_family = PF_INET6;
    addr.sin6_addr = ip;
    addr.sin6_port = port;
#if !defined(NDEBUG) && !defined(UC_VERSION)
    char addrbuf [128];
    DEBUG_PRINTF ("[X] connecting to socket %s:%hu\n", inet_ntop(PF_INET6, &addr.sin6_addr, ARRAY_BLOCK(addrbuf)), port);
#endif
    return connect (pointer_cast<sockaddr>(&addr), sizeof(addr));
}

/// Create local IPv6 socket at given ip and port
auto PExtern::connect_local_ip6 (in_port_t port) noexcept -> fd_t
{
    sockaddr_in6 addr = {};
    addr.sin6_family = PF_INET6;
    addr.sin6_addr = IN6ADDR_LOOPBACK_INIT;
    addr.sin6_port = port;
    DEBUG_PRINTF ("[X] connecting to socket localhost6:%hu\n", port);
    return connect (pointer_cast<sockaddr>(&addr), sizeof(addr));
}

auto PExtern::launch_pipe (const char* exe, const char* arg) noexcept -> fd_t
{
    // Check if executable exists before the fork to allow proper error handling
    char exepath [PATH_MAX];
    auto exefp = executable_in_path (exe, ARRAY_BLOCK(exepath));
    if (!exefp) {
	errno = ENOENT;
	return -1;
    }

    // Create socket pipe, will be connected to stdin in server
    enum { socket_ClientSide, socket_ServerSide, socket_N };
    fd_t socks [socket_N];
    if (0 > socketpair (PF_LOCAL, SOCK_STREAM| SOCK_NONBLOCK, 0, socks))
	return -1;

    if (auto fr = fork(); fr < 0) {
	::close (socks[socket_ClientSide]);
	::close (socks[socket_ServerSide]);
	return -1;
    } else if (!fr) {	// Server side
	::close (socks[socket_ClientSide]);
	dup2 (socks[socket_ServerSide], STDIN_FILENO);
	execl (exefp, exe, arg, NULL);

	// If exec failed, log the error and exit
	Msger::error ("failed to launch pipe to '%s %s': %s\n", exe, arg, strerror(errno));
	exit (EXIT_FAILURE);
    }
    // Client side
    ::close (socks[socket_ServerSide]);
    open (socks[socket_ClientSide]);
    return socks[socket_ClientSide];
}

//}}}-------------------------------------------------------------------
//{{{ PExternR

DEFINE_INTERFACE (ExternR)

//}}}-------------------------------------------------------------------
//{{{ Extern

Extern::Extern (const Msg::Link& l) noexcept
: Msger (l)
,_sockfd (-1)
,_timer (msger_id())
,_reply (l)
,_bwritten (0)
,_outq()
,_relays()
,_einfo{}
,_bread (0)
,_inmsg()
,_infd (-1)
{
    _relays.emplace_back (msger_id(), msger_id(), extid_COM);
    extern_list().push_back (this);
}

Extern::~Extern (void) noexcept
{
    Extern_close();
    remove (extern_list(), this);
}

bool Extern::dispatch (Msg& msg) noexcept
{
    return PTimerR::dispatch (this,msg)
	|| PExtern::dispatch (this,msg)
	|| PCOM::dispatch (this,msg)
	|| Msger::dispatch (msg);
}

void Extern::queue_outgoing (methodid_t mid, Msg::Body&& body, Msg::fdoffset_t fdo, mrid_t extid) noexcept
{
    _outq.emplace_back (mid, move(body), fdo, extid);
    TimerR_timer (_sockfd);
}

Extern::RelayProxy* Extern::relay_proxy_by_id (mrid_t id) noexcept
{
    return linear_search_if (_relays, [&](const auto& r)
	    { return r.relay.dest() == id; });
}

Extern::RelayProxy* Extern::relay_proxy_by_extid (mrid_t extid) noexcept
{
    return linear_search_if (_relays, [&](const auto& r)
	    { return r.extid == extid; });
}

mrid_t Extern::register_relay (const COMRelay* relay) noexcept
{
    auto rp = relay_proxy_by_id (relay->msger_id());
    if (!rp)
	rp = &_relays.emplace_back (msger_id(), relay->msger_id(), create_extid_from_relay_id (relay->msger_id()));
    rp->pRelay = relay;
    return rp->extid;
}

void Extern::unregister_relay (const COMRelay* relay) noexcept
{
    auto rp = relay_proxy_by_id (relay->msger_id());
    if (rp)
	_relays.erase (rp);
}

Extern* Extern::lookup_by_id (mrid_t id) noexcept // static
{
    auto ep = linear_search_if (extern_list(), [&](const auto e)
		{ return e->msger_id() == id; });
    return ep ? *ep : nullptr;
}

Extern* Extern::lookup_by_imported (iid_t iid) noexcept // static
{
    auto ep = linear_search_if (extern_list(), [&](const auto e)
		{ return e->info().is_importing(iid); });
    return ep ? *ep : nullptr;
}

Extern* Extern::lookup_by_relay_id (mrid_t rid) noexcept // static
{
    auto ep = linear_search_if (extern_list(), [&](const auto e)
		{ return nullptr != e->relay_proxy_by_id (rid); });
    return ep ? *ep : nullptr;
}

//}}}-------------------------------------------------------------------
//{{{ Extern::ExtMsg

Extern::ExtMsg::ExtMsg (methodid_t mid, Msg::Body&& body, Msg::fdoffset_t fdo, mrid_t extid) noexcept
:_body (move(body))
,_h { ceilg (_body.size(), Msg::Alignment::Body), extid, fdo, write_header_strings(mid) }
{
    assert (_body.capacity() >= _h.sz && "message body must be created aligned to Msg::Alignment::Body");
    _body.shrink (_h.sz);
}

void Extern::ExtMsg::write_iovecs (iovec* iov, streamsize bw) noexcept
{
    // Setup the two iovecs, 0 for header, 1 for body
    // bw is the bytes already written in previous sendmsg call
    auto hp = header_ptr();	// char* to full header
    auto hsz = _h.hsz + sizeof(_h)*!_h.hsz;
    if (bw < hsz) {	// still need to write header
	hsz -= bw;
	hp += bw;
	bw = 0;
    } else {		// header already written
	bw -= hsz;
	hp = nullptr;
	hsz = 0;
    }
    iov[0].iov_base = hp;
    iov[0].iov_len = hsz;
    iov[1].iov_base = _body.iat(bw);
    iov[1].iov_len = _h.sz - bw;
}

methodid_t Extern::ExtMsg::parse_method (void) const noexcept
{
    czstri mi (_hbuf, _h.hsz-sizeof(_h));
    auto ifacename = *mi;
    auto methodname = *++mi;
    if (!mi.remaining())
	return nullptr;
    if (!(++mi).remaining())
	return nullptr;
    auto methodend = *++mi;
    auto methodnamesz = methodend - methodname;
    auto iface = App::interface_by_name (ifacename, methodname-ifacename);
    if (!iface)
	return nullptr;
    return interface_lookup_method (iface, methodname, methodnamesz);
}

void Extern::ExtMsg::debug_dump (void) const noexcept
{
    if (DEBUG_MSG_TRACE) {
	DEBUG_PRINTF ("[X] Incoming message for extid %u of size %u = {{{\n", _h.extid, _h.sz);
	hexdump (header_ptr(), _h.hsz);
	hexdump (_body);
	DEBUG_PRINTF ("}}}\n");
    }
}

//}}}-------------------------------------------------------------------
//{{{ Extern::Extern

void Extern::Extern_open (fd_t fd, const iid_t* eifaces, PExtern::SocketSide side) noexcept
{
    if (!attach_to_socket (fd))
	return error ("invalid socket type");
    _sockfd = fd;
    _einfo.exported = eifaces;
    _einfo.side = side;
    enable_credentials_passing (true);
    // Initial handshake is an exchange of COM::export messages
    queue_outgoing (PCOM::export_msg (eifaces), extid_COM);
}

void Extern::Extern_close (void) noexcept
{
    set_unused();
    close (exchange (_sockfd, -1));
}

bool Extern::attach_to_socket (fd_t fd) noexcept
{
    // The incoming socket must be a stream socket
    int v;
    socklen_t l = sizeof(v);
    if (getsockopt (fd, SOL_SOCKET, SO_TYPE, &v, &l) < 0 || v != SOCK_STREAM)
	return false;

    // And it must match the family (PF_LOCAL or PF_INET)
    sockaddr_storage ss;
    l = sizeof(ss);
    if (getsockname (fd, reinterpret_cast<sockaddr*>(&ss), &l) < 0)
	return false;
    _einfo.is_unix_socket = false;
    if (ss.ss_family == PF_LOCAL)
	_einfo.is_unix_socket = true;
    else if (ss.ss_family != PF_INET)
	return false;

    // If matches, need to set the fd nonblocking for the poll loop to work
    return !PTimer::make_nonblocking (fd);
}

void Extern::enable_credentials_passing (bool enable) noexcept
{
    if (_sockfd < 0 || !_einfo.is_unix_socket)
	return;
    int sov = enable;
    if (0 > setsockopt (_sockfd, SOL_SOCKET, SO_PASSCRED, &sov, sizeof(sov)))
	return error_libc ("setsockopt(SO_PASSCRED)");
}

//}}}-------------------------------------------------------------------
//{{{ Extern::COM

void Extern::COM_error (const string_view& errmsg) noexcept
{
    // errors occuring on in the Extern Msger on the other side of the socket
    error ("%s", errmsg.c_str());	// report it on this side
}

void Extern::COM_export (string elist) noexcept
{
    // Other side of the socket listing exported interfaces as a comma-separated list
    _einfo.imported.clear();
    foreach (ei, elist) {
	auto eic = elist.find (',', ei);
	if (!eic)
	    eic = elist.end();
	*eic++ = 0;
	auto iid = App::interface_by_name (ei, eic-ei);
	if (iid)	// _einfo.imported only contains interfaces supported by this App
	    _einfo.imported.push_back (iid);
	ei = eic;
    }
    _reply.connected (&_einfo);
}

void Extern::COM_delete (void) noexcept
{
    // This happens when the Extern Msger on the other side of the socket dies
    set_unused();
}

//}}}-------------------------------------------------------------------
//{{{ Extern::Timer

void Extern::TimerR_timer (fd_t) noexcept
{
    if (_sockfd >= 0)
	read_incoming();
    auto tcmd = PTimer::WatchCmd::Read;
    if (_sockfd >= 0 && write_outgoing())
	tcmd = PTimer::WatchCmd::ReadWrite;
    if (_sockfd >= 0)
	_timer.watch (tcmd, _sockfd);
}

//{{{2 write_outgoing ---------------------------------------------------

// writes queued messages. Returns true if need to wait for write.
bool Extern::write_outgoing (void) noexcept
{
    // write all queued messages
    while (!_outq.empty()) {
	// Build sendmsg header
	msghdr mh = {};

	// Add fd if being passed
	auto passedfd = _outq.front().passed_fd();
	char fdbuf [CMSG_SPACE(sizeof(passedfd))] = {};
	unsigned nm = (passedfd >= 0);
	if (nm && !_bwritten) {	// only the first write passes the fd
	    mh.msg_control = fdbuf;
	    mh.msg_controllen = sizeof(fdbuf);
	    auto cmsg = CMSG_FIRSTHDR(&mh);
	    cmsg->cmsg_len = sizeof(fdbuf);
	    cmsg->cmsg_level = SOL_SOCKET;
	    cmsg->cmsg_type = SCM_RIGHTS;
	    ostream (CMSG_DATA (cmsg), sizeof(passedfd)) << passedfd;
	}

	// See how many messages can be written at once, limited by fd passing.
	// Can only pass one fd per sendmsg call, but can aggregate the rest.
	while (nm < _outq.size() && !_outq[nm].has_fd())
	    ++nm;

	// Create iovecs for output
	iovec iov [2*nm];	// two iovecs per message, header and body
	mh.msg_iov = iov;
	mh.msg_iovlen = 2*nm;
	for (auto m = 0u, bw = _bwritten; m < nm; ++m, bw = 0)
	    _outq[m].write_iovecs (&iov[2*m], bw);

	// And try writing it all
	if (auto smr = sendmsg (_sockfd, &mh, MSG_NOSIGNAL); smr <= 0) {
	    if (!smr || errno == ECONNRESET)	// smr == 0 when remote end closes. No error then, just need to close this end too.
		DEBUG_PRINTF ("[X] %hu.Extern: wsocket %d closed by the other end\n", msger_id(), _sockfd);
	    else if (errno == EINTR)
		continue;
	    else if (errno == EAGAIN)
		return true;
	    else
		error_libc ("sendmsg");
	    Extern_close();
	    return false;
	} else { // At this point sendmsg has succeeded and wrote some bytes
	    DEBUG_PRINTF ("[X] Wrote %ld bytes to socket %d\n", smr, _sockfd);
	    _bwritten += smr;
	}

	// Close the fd once successfully passed
	if (passedfd >= 0)
	    close (passedfd);

	// Erase messages that have been fully written
	auto ndone = 0u;
	for (; ndone < nm && _bwritten >= _outq[ndone].size(); ++ndone)
	    _bwritten -= _outq[ndone].size();
	_outq.erase (_outq.begin(), ndone);

	assert (((_outq.empty() && !_bwritten) || (_bwritten < _outq.front().size()))
		&& "_bwritten must now be equal to bytes written from first message in queue");
    }
    return false;
}

//}}}2------------------------------------------------------------------
//{{{2 read_incoming

void Extern::read_incoming (void) noexcept
{
    for (;;) {	// Read until EAGAIN
	// Create iovecs for input
	// There are three of them, representing the header and the body
	// of each message, plus the fixed header of the next. The common
	// case is to read the variable parts and the fixed header of the
	// next message in each recvmsg call.
	ExtMsg::Header fh = {};
	iovec iov[3] = {{},{},{&fh,sizeof(fh)}};
	_inmsg.write_iovecs (iov, _bread);

	// Ancillary space for fd and credentials
	char cmsgbuf [CMSG_SPACE(sizeof(_infd)) + CMSG_SPACE(sizeof(ucred))] = {};

	// Build struct for recvmsg
	msghdr mh = {};
	mh.msg_iov = iov;
	mh.msg_iovlen = 2 + (_bread >= sizeof(fh));	// read another header only when already have one
	mh.msg_control = cmsgbuf;
	mh.msg_controllen = sizeof(cmsgbuf);

	// Receive some data
	if (auto rmr = recvmsg (_sockfd, &mh, 0); rmr <= 0) {
	    if (!rmr || errno == ECONNRESET)	// br == 0 when remote end closes. No error then, just need to close this end too.
		DEBUG_PRINTF ("[X] %hu.Extern: rsocket %d closed by the other end\n", msger_id(), _sockfd);
	    else if (errno == EINTR)
		continue;
	    else if (errno == EAGAIN)
		return;			// <--- the usual exit point
	    else
		error_libc ("recvmsg");
	    return Extern_close();
	} else {
	    DEBUG_PRINTF ("[X] %hu.Extern: read %ld bytes from socket %d\n", msger_id(), rmr, _sockfd);
	    _bread += rmr;
	}

	// Check if ancillary data was passed
	for (auto cmsg = CMSG_FIRSTHDR(&mh); cmsg; cmsg = CMSG_NXTHDR(&mh, cmsg)) {
	    if (cmsg->cmsg_type == SCM_CREDENTIALS) {
		istream (CMSG_DATA(cmsg), sizeof(ucred)) >> _einfo.creds;
		enable_credentials_passing (false);	// Credentials only need to be received once
		DEBUG_PRINTF ("[X] Received credentials: pid=%u,uid=%u,gid=%u\n", _einfo.creds.pid, _einfo.creds.uid, _einfo.creds.gid);
	    } else if (cmsg->cmsg_type == SCM_RIGHTS) {
		if (_infd >= 0) {	// if message is sent in multiple parts, the fd must only be sent with the first piece
		    error ("multiple file descriptors received in one message");
		    return Extern_close();
		}
		istream (CMSG_DATA(cmsg), sizeof(_infd)) >> _infd;
		DEBUG_PRINTF ("[X] Received fd %d\n", _infd);
	    }
	}

	// If the read message is complete, validate it and queue for delivery
	if (_bread >= _inmsg.size()) {
	    _bread -= _inmsg.size();
	    _inmsg.debug_dump();

	    // write the passed fd into the body
	    if (_inmsg.has_fd()) {
		assert (_infd >= 0 && "_infd magically disappeared since header check");
		_inmsg.set_passed_fd (exchange (_infd, -1));
	    }

	    if (!accept_incoming_message()) {
		error ("invalid message");
		return Extern_close();
	    }

	    // Copy the fixed header of the next message
	    _inmsg.set_header (fh);
	    assert (_bread <= sizeof(fh) && "recvmsg read unrequested data");
	}
	// Here, a message has been accepted, or there was no message,
	// and there is a partially or fully read fixed header of another.

	// Now can check if fixed header is valid
	if (_bread == sizeof(fh)) {
	    auto& h = _inmsg.header();
	    if (h.hsz < ExtMsg::MinHeaderSize
		    || !divisible_by (h.hsz, Msg::Alignment::Header)
		    || h.sz > ExtMsg::MaxBodySize
		    || !divisible_by (h.sz, Msg::Alignment::Body)
		    || (h.fdoffset != Msg::NoFdIncluded
			&& (_infd < 0	// the fd must be passed at this point
			    || h.fdoffset+sizeof(_infd) > h.sz
			    || !divisible_by (h.fdoffset, Msg::Alignment::Fd)))
		    || h.extid > extid_ServerLast) {
		error ("invalid message");
		return Extern_close();
	    }
	    _inmsg.allocate_body (h.sz);
	}
    }
}

bool Extern::accept_incoming_message (void) noexcept
{
    // Validate the message using method signature
    auto method = _inmsg.parse_method();
    if (!method) {
	DEBUG_PRINTF ("[XE] Incoming message has invalid header strings\n");
	return false;
    }
    auto msgis = _inmsg.read();
    auto vsz = Msg::validate_signature (msgis, signature_of_method (method));
    if (ceilg (vsz, Msg::Alignment::Body) != _inmsg.body_size()) {
	DEBUG_PRINTF ("[XE] Incoming message body failed validation\n");
	return false;
    }
    _inmsg.trim_body (vsz);	// Local messages store unpadded size

    // Lookup or create local relay proxy
    auto rp = relay_proxy_by_extid (_inmsg.extid());
    if (!rp) {
	// Verify that the requested interface is on the exported list
	if (!_einfo.is_exporting (interface_of_method (method))) {
	    DEBUG_PRINTF ("[XE] Incoming message requests unexported interface\n");
	    return false;
	}
	// Verify that the other side sets extid correctly
	if (bool(_einfo.side) ^ (_inmsg.extid() < extid_ServerBase)) {
	    DEBUG_PRINTF ("[XE] Extern connection peer allocates incorrect extids\n");
	    return false;
	}
	DEBUG_PRINTF ("[X] Creating new extid link %hu\n", _inmsg.extid());
	rp = &_relays.emplace_back (msger_id(), mrid_New, _inmsg.extid());
	//
	// Create a COMRelay as the destination. It will then create the
	// actual server Msger using the interface in the message.
	rp->relay.create_dest_as (PCOM::interface());
    }

    // Create local message from ExtMsg and forward it to the COMRelay
    rp->relay.forward_msg (Msg (rp->relay.link(), method, _inmsg.move_body(), _inmsg.fd_offset(), _inmsg.extid()));
    return true;
}
//}}}2
//}}}-------------------------------------------------------------------
//{{{ COMRelay

COMRelay::COMRelay (const Msg::Link& l) noexcept
: Msger (l)
//
// COMRelays can be created either by local callers sending messages to
// imported interfaces, or by an Extern delivering messages to local
// instances of exported interfaces.
//
,_pExtern (Extern::lookup_by_id (l.src))
//
// Messages coming from an extern will require creating a local Msger,
// while messages going to the extern from l.src local caller.
//
,_localp (l.dest, _pExtern ? mrid_t (mrid_New) : l.src)
//
// Extid will be determined when the connection interface is known
,_extid()
{
}

COMRelay::~COMRelay (void) noexcept
{
    // The relay is destroyed when:
    // 1. The local Msger is destroyed. COM delete message is sent to the
    //    remote side as notification.
    // 2. The remote object is destroyed. The relay is marked unused in
    //    COMRelay_COM_delete and the extern pointer is reset to prevent
    //    further messages to remote object. Here, no message is sent.
    // 3. The Extern object is destroyed. pExtern is reset in
    //    COMRelay_ObjectDestroyed, and no message is sent here.
    if (_pExtern) {
	if (_extid)
	    _pExtern->queue_outgoing (PCOM::delete_msg(), _extid);
	_pExtern->unregister_relay (this);
    }
    _pExtern = nullptr;
    _extid = 0;
}

bool COMRelay::dispatch (Msg& msg) noexcept
{
    // COM messages are processed here
    if (PCOM::dispatch (this, msg))
	return true;

    // Messages to imported interfaces need to be routed to the Extern
    // that imports it. The interface was unavailable in ctor, so here.
    if (!_pExtern) {	// If null here, then this relay was created by a local Msger
	auto iface = msg.interface();
	if (!(_pExtern = Extern::lookup_by_imported (iface))) {
	    error ("interface %s has not been imported", iface);
	    return false;	// the caller should have waited for Extern connected reply before creating this
	}
    }
    // Now that the interface is known and extern pointer is available,
    // the relay can register and obtain a connection extid.
    if (!_extid) {
	_extid = _pExtern->register_relay (this);
	//
	// If remote object was marked unused, but the local caller has recreated it
	// through this same relay before it was destroyed, f_Unused needs to be reset.
	//
	set_unused (false);
    }

    // Forward the message in the direction opposite which it was received
    if (msg.src() == _localp.dest())
	_pExtern->queue_outgoing (move(msg), _extid);
    else {
	assert (msg.extid() == _extid && "Extern routed a message to the wrong relay");
	_localp.forward_msg (move(msg));
    }
    return true;
}

bool COMRelay::on_error (mrid_t eid, const string& errmsg) noexcept
{
    // An unhandled error in the local object is forwarded to the remote
    // object. At this point it will be considered handled. The remote
    // will decide whether to delete itself, which will propagate here.
    //
    if (_pExtern && eid == _localp.dest()) {
	DEBUG_PRINTF ("[X] COMRelay forwarding error to extern creator\n");
	_pExtern->queue_outgoing (PCOM::error_msg (errmsg), _extid);
	return true;	// handled on the remote end.
    }
    // errors occuring in the Extern object or elsewhere can not be handled
    // by forwarding, so fall back to default handling.
    return Msger::on_error (eid, errmsg);
}

void COMRelay::on_msger_destroyed (mrid_t id) noexcept
{
    // When the Extern object is destroyed, this notification arrives from
    // the App when the Extern created this relay. Relays created by local
    // Msgers will be manually notified by the Extern being deleted. In the
    // first case, the Extern object is available to send the COM Destroy
    // notification, in the second, it is not.
    //
    if (id != _localp.dest())
	_pExtern = nullptr;	// when not, do not try to send the message

    // In both cases, the relay can no longer function, and so is deleted
    set_unused();
}

void COMRelay::COM_error (const string_view& errmsg) noexcept
{
    // COM_error is received for errors in the remote object. The remote
    // object is destroyed and COM_delete will shortly follow. Here, create
    // a local error and send it to the local object.
    //
    error ("%s", errmsg.c_str());
 
    // Because the local object may not be the creator of this relay, the
    // error must be forwarded there manually.
    //
    App::instance().forward_error (_localp.dest(), _localp.src());
}

void COMRelay::COM_export (const string_view&) noexcept
{
    // Relays never receive this message
}

void COMRelay::COM_delete (void) noexcept
{
    // COM_delete indicates that the remote object has been destroyed.
    _pExtern = nullptr;	// No further messages are to be sent.
    _extid = 0;
    set_unused();	// The relay and local object are to be destroyed.
}

//}}}-------------------------------------------------------------------
//{{{ PExternServer

PExternServer::~PExternServer (void) noexcept
{
    if (!_sockname.empty())
	unlink (_sockname.c_str());
}

/// Create server socket bound to the given address
auto PExternServer::bind (const sockaddr* addr, socklen_t addrlen, const iid_t* eifaces) noexcept -> fd_t
{
    auto fd = socket (addr->sa_family, SOCK_STREAM| SOCK_NONBLOCK| SOCK_CLOEXEC, IPPROTO_IP);
    if (fd < 0)
	return fd;
    if (0 > ::bind (fd, addr, addrlen) && errno != EINPROGRESS) {
	DEBUG_PRINTF ("[E] Failed to bind to socket: %s\n", strerror(errno));
	::close (fd);
	return -1;
    }
    if (0 > listen (fd, SOMAXCONN)) {
	DEBUG_PRINTF ("[E] Failed to listen to socket: %s\n", strerror(errno));
	::close (fd);
	return -1;
    }
    if (addr->sa_family == PF_LOCAL)
	_sockname = pointer_cast<sockaddr_un>(addr)->sun_path;
    open (fd, eifaces, WhenEmpty::Remain);
    return fd;
}

/// Create local socket with given path
auto PExternServer::bind_local (const char* path, const iid_t* eifaces) noexcept -> fd_t
{
    sockaddr_un addr;
    addr.sun_family = PF_LOCAL;
    if (size(addr.sun_path) <= unsigned (snprintf (ARRAY_BLOCK(addr.sun_path), "%s", path))) {
	errno = ENAMETOOLONG;
	return -1;
    }
    DEBUG_PRINTF ("[X] Creating server socket %s\n", addr.sun_path);
    auto fd = bind (pointer_cast<sockaddr>(&addr), sizeof(addr), eifaces);
    if (0 > chmod (addr.sun_path, DEFFILEMODE))
	DEBUG_PRINTF ("[E] Failed to change socket permissions: %s\n", strerror(errno));
    return fd;
}

/// Create local socket of the given name in the system standard location for such
auto PExternServer::bind_system_local (const char* sockname, const iid_t* eifaces) noexcept -> fd_t
{
    sockaddr_un addr;
    addr.sun_family = PF_LOCAL;
    if (size(addr.sun_path) <= unsigned (snprintf (ARRAY_BLOCK(addr.sun_path), _PATH_VARRUN "%s", sockname))) {
	errno = ENAMETOOLONG;
	return -1;
    }
    DEBUG_PRINTF ("[X] Creating server socket %s\n", addr.sun_path);
    auto fd = bind (pointer_cast<sockaddr>(&addr), sizeof(addr), eifaces);
    if (0 > chmod (addr.sun_path, DEFFILEMODE))
	DEBUG_PRINTF ("[E] Failed to change socket permissions: %s\n", strerror(errno));
    return fd;
}

/// Create local socket of the given name in the user standard location for such
auto PExternServer::bind_user_local (const char* sockname, const iid_t* eifaces) noexcept -> fd_t
{
    sockaddr_un addr;
    addr.sun_family = PF_LOCAL;
    const char* runtimeDir = getenv ("XDG_RUNTIME_DIR");
    if (!runtimeDir)
	runtimeDir = _PATH_TMP;
    if (size(addr.sun_path) <= unsigned (snprintf (ARRAY_BLOCK(addr.sun_path), "%s/%s", runtimeDir, sockname))) {
	errno = ENAMETOOLONG;
	return -1;
    }
    DEBUG_PRINTF ("[X] Creating server socket %s\n", addr.sun_path);
    auto fd = bind (pointer_cast<sockaddr>(&addr), sizeof(addr), eifaces);
    if (0 > chmod (addr.sun_path, S_IRUSR| S_IWUSR))
	DEBUG_PRINTF ("[E] Failed to change socket permissions: %s\n", strerror(errno));
    return fd;
}

/// Create local IPv4 socket at given ip and port
auto PExternServer::bind_ip4 (in_addr_t ip, in_port_t port, const iid_t* eifaces) noexcept -> fd_t
{
    sockaddr_in addr = {};
    addr.sin_family = PF_INET,
    #ifdef UC_VERSION
	addr.sin_addr = ip;
    #else
	addr.sin_addr = { ip };
    #endif
    addr.sin_port = port;
    return bind (pointer_cast<sockaddr>(&addr), sizeof(addr), eifaces);
}

/// Create local IPv4 socket at given port on the loopback interface
auto PExternServer::bind_local_ip4 (in_port_t port, const iid_t* eifaces) noexcept -> fd_t
    { return bind_ip4 (INADDR_LOOPBACK, port, eifaces); }

/// Create local IPv6 socket at given ip and port
auto PExternServer::bind_ip6 (in6_addr ip, in_port_t port, const iid_t* eifaces) noexcept -> fd_t
{
    sockaddr_in6 addr = {};
    addr.sin6_family = PF_INET6;
    addr.sin6_addr = ip;
    addr.sin6_port = port;
    return bind (pointer_cast<sockaddr>(&addr), sizeof(addr), eifaces);
}

/// Create local IPv6 socket at given ip and port
auto PExternServer::bind_local_ip6 (in_port_t port, const iid_t* eifaces) noexcept -> fd_t
{
    sockaddr_in6 addr = {};
    addr.sin6_family = PF_INET6;
    addr.sin6_addr = IN6ADDR_LOOPBACK_INIT;
    addr.sin6_port = port;
    return bind (pointer_cast<sockaddr>(&addr), sizeof(addr), eifaces);
}

//}}}-------------------------------------------------------------------
//{{{ ExternServer

bool ExternServer::on_error (mrid_t eid, const string& errmsg) noexcept
{
    if (_timer.dest() == eid || msger_id() == eid)
	return false;	// errors in listened socket are fatal
    // error in accepted socket. Handle by logging the error and removing record in ObjectDestroyed.
    syslog (LOG_ERR, "%s", errmsg.c_str());
    return true;
}

void ExternServer::on_msger_destroyed (mrid_t mid) noexcept
{
    DEBUG_PRINTF ("[X] Client connection %hu dropped\n", mid);
    remove_if (_conns, [&](const auto& e) { return e.dest() == mid; });
    if (_conns.empty() && flag (f_CloseWhenEmpty))
	set_unused();
}

bool ExternServer::dispatch (Msg& msg) noexcept
{
    return PTimerR::dispatch (this, msg)
	|| PExternServer::dispatch (this, msg)
	|| PExternR::dispatch (this, msg)
	|| Msger::dispatch (msg);
}

void ExternServer::TimerR_timer (PTimer::fd_t) noexcept
{
    for (int cfd; 0 <= (cfd = accept (_sockfd, nullptr, nullptr));) {
	DEBUG_PRINTF ("[X] Client connection accepted on fd %d\n", cfd);
	_conns.emplace_back (msger_id()).open (cfd, _eifaces);
	set_unused (false);
    }
    if (errno == EAGAIN) {
	DEBUG_PRINTF ("[X] Resuming wait on fd %d\n", _sockfd);
	_timer.wait_read (_sockfd);
    } else {
	DEBUG_PRINTF ("[X] Accept failed with error %s\n", strerror(errno));
	error_libc ("accept");
    }
}

void ExternServer::ExternServer_open (int fd, const iid_t* eifaces, PExternServer::WhenEmpty closeWhenEmpty) noexcept
{
    assert (_sockfd == -1 && "each ExternServer instance can only listen to one socket");
    if (PTimer::make_nonblocking (fd))
	return error_libc ("fcntl(SETFL(O_NONBLOCK))");
    _sockfd = fd;
    _eifaces = eifaces;
    set_flag (f_CloseWhenEmpty, bool(closeWhenEmpty));
    TimerR_timer (_sockfd);
}

void ExternServer::ExternServer_close (void) noexcept
{
    set_unused();
    _timer.stop();
}

void ExternServer::ExternR_connected (const Extern::Info* einfo) noexcept
{
    _reply.connected (einfo);
}

} // namespace cwiclo
//}}}-------------------------------------------------------------------
