// This file is part of the casycom project
//
// Copyright (c) 2015 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "xcom.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/un.h>

//{{{ COM --------------------------------------------------------------
namespace cwiclo {

string PCOM::string_from_interface_list (const iid_t* elist) // static
{
    string elstr;
    if (elist && *elist) {
	for (auto e = elist; *e; ++e)
	    elstr.appendf ("%s,", *e);
	elstr.pop_back();
    }
    return elstr;
}

Msg PCOM::export_msg (const string& elstr) // static
{
    Msg msg (Msg::Link{}, PCOM::m_export(), stream_sizeof(elstr), Msg::NoFdIncluded);
    msg.write() << elstr;
    return msg;
}

Msg PCOM::error_msg (const string& errmsg) // static
{
    Msg msg (Msg::Link{}, PCOM::m_error(), stream_sizeof(errmsg), Msg::NoFdIncluded);
    msg.write() << errmsg;
    return msg;
}

//}}}-------------------------------------------------------------------
//{{{ PExtern

auto PExtern::connect (const sockaddr* addr, socklen_t addrlen) const -> fd_t
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

/// Create local socket with given path
auto PExtern::connect_local (const char* path) const -> fd_t
{
    sockaddr_un addr;
    auto addrlen = create_sockaddr_local (&addr, path);
    if (addrlen < 0)
	return addrlen;
    return connect (pointer_cast<sockaddr>(&addr), addrlen);
}

/// Create local socket of the given name in the system standard location for such
auto PExtern::connect_system_local (const char* sockname) const -> fd_t
{
    sockaddr_un addr;
    auto addrlen = create_sockaddr_system_local (&addr, sockname);
    if (addrlen < 0)
	return addrlen;
    return connect (pointer_cast<sockaddr>(&addr), addrlen);
}

/// Create local socket of the given name in the user standard location for such
auto PExtern::connect_user_local (const char* sockname) const -> fd_t
{
    sockaddr_un addr;
    auto addrlen = create_sockaddr_user_local (&addr, sockname);
    if (addrlen < 0)
	return addrlen;
    return connect (pointer_cast<sockaddr>(&addr), addrlen);
}

auto PExtern::launch_pipe (const char* exe, const char* arg) const -> fd_t
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

//}}}-------------------------------------------------------------------
//{{{ Extern

Extern::Extern (const Msg::Link& l)
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

Extern::~Extern (void)
{
    Extern_close();
    remove (extern_list(), this);
    // Outgoing connections do not create a link from relay to extern and so need to be notified explicitly of extern's destruction.
    for (auto& rp : _relays) {
	if (rp.pRelay)
	    rp.pRelay->on_msger_destroyed (msger_id());
	rp.pRelay = nullptr;
    }
}

bool Extern::dispatch (Msg& msg)
{
    return PTimer::Reply::dispatch (this,msg)
	|| PExtern::dispatch (this,msg)
	|| PCOM::dispatch (this,msg)
	|| Msger::dispatch (msg);
}

void Extern::queue_outgoing (methodid_t mid, Msg::Body&& body, Msg::fdoffset_t fdo, extid_t extid)
{
    _outq.emplace_back (mid, move(body), fdo, extid);
    Timer_timer (_sockfd);
}

Extern::RelayProxy* Extern::relay_proxy_by_id (mrid_t id)
{
    return find_if (_relays, [&](const auto& r)
	    { return r.relay.dest() == id; });
}

Extern::RelayProxy* Extern::relay_proxy_by_extid (extid_t extid)
{
    return find_if (_relays, [&](const auto& r)
	    { return r.extid == extid; });
}

extid_t Extern::register_relay (COMRelay* relay)
{
    auto rp = relay_proxy_by_id (relay->msger_id());
    if (!rp)
	rp = &_relays.emplace_back (msger_id(), relay->msger_id(), create_extid_from_relay_id (relay->msger_id()));
    rp->pRelay = relay;
    return rp->extid;
}

void Extern::unregister_relay (const COMRelay* relay)
{
    auto rp = relay_proxy_by_id (relay->msger_id());
    if (rp)
	_relays.erase (rp);
}

Extern* Extern::lookup_by_id (mrid_t id) // static
{
    auto ep = find_if (extern_list(), [&](const auto e)
		{ return e->msger_id() == id; });
    return ep ? *ep : nullptr;
}

Extern* Extern::lookup_by_imported (iid_t iid) // static
{
    auto ep = find_if (extern_list(), [&](const auto e)
		{ return e->info().is_importing(iid); });
    return ep ? *ep : nullptr;
}

Extern* Extern::lookup_by_relay_id (mrid_t rid) // static
{
    auto ep = find_if (extern_list(), [&](const auto e)
		{ return nullptr != e->relay_proxy_by_id (rid); });
    return ep ? *ep : nullptr;
}

//}}}-------------------------------------------------------------------
//{{{ Extern::ExtMsg

Extern::ExtMsg::ExtMsg (methodid_t mid, Msg::Body&& body, Msg::fdoffset_t fdo, extid_t extid)
:_body (move(body))
,_h { ceilg (_body.size(), Msg::Alignment::Body), extid, fdo, write_header_strings(mid) }
{
    assert (_body.capacity() >= _h.sz && "message body must be created aligned to Msg::Alignment::Body");
    _body.shrink (_h.sz);
}

uint8_t Extern::ExtMsg::write_header_strings (methodid_t method)
{
    // _hbuf contains iface\0method\0signature\0, padded to Msg::Alignment::Header
    auto iface = interface_of_method (method);
    assert (ptrdiff_t(sizeof(_hbuf)) >= interface_name_size(iface)+method_name_size(method) && "the interface and method names for this message are too long to export");
    ostream os (_hbuf);
    os.write (iface, interface_name_size (iface));
    os.write (method, method_name_size(method));
    os.align (Msg::Alignment::Header);
    return sizeof(_h) + os.ptr()-begin(_hbuf);
}

void Extern::ExtMsg::write_iovecs (iovec* iov, streamsize bw)
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

methodid_t Extern::ExtMsg::parse_method (void) const
{
    zstr::cii mi (_hbuf, _h.hsz-sizeof(_h));
    auto ifacename = *mi;
    auto methodname = *++mi;
    if (!mi.remaining())
	return nullptr;
    if (!(++mi).remaining())
	return nullptr;
    auto methodend = *++mi;
    auto methodnamesz = methodend - methodname;
    auto iface = App::interface_by_name (ifacename, methodname-ifacename);
    if (!iface) {
	DEBUG_PRINTF ("[XE] Extern message arrived for %s.%s, but the interface is not registered.\n\tDid you forget to REGISTER_EXTERN_MSGER it?\n\tRemember that reply interfaces must also be registered.\n", ifacename, methodname);
	return nullptr;
    }
    return interface_lookup_method (iface, methodname, methodnamesz);
}

void Extern::ExtMsg::debug_dump (void) const
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

void Extern::Extern_open (fd_t fd, const iid_t* eifaces, PExtern::SocketSide side)
{
    if (!attach_to_socket (fd))
	return error ("invalid socket type");
    _sockfd = fd;
    _einfo.extern_id = msger_id();
    _einfo.exported = eifaces;
    _einfo.side = side;
    enable_credentials_passing (true);
    // Initial handshake is an exchange of COM::export messages
    queue_outgoing (PCOM::export_msg (eifaces), extid_COM);
}

void Extern::Extern_close (void)
{
    set_unused();
    if (_infd >= 0)
	close (exchange (_infd, -1));
    if (_sockfd >= 0)
	close (exchange (_sockfd, -1));
}

bool Extern::attach_to_socket (fd_t fd)
{
    // The incoming socket must be a stream socket
    int v;
    socklen_t l = sizeof(v);
    if (getsockopt (fd, SOL_SOCKET, SO_TYPE, &v, &l) < 0 || v != SOCK_STREAM)
	return false;

    // And it must match the family (PF_LOCAL or PF_INET)
    sockaddr_storage ss;
    l = sizeof(ss);
    if (getsockname (fd, pointer_cast<sockaddr>(&ss), &l) < 0)
	return false;
    _einfo.is_unix_socket = false;
    if (ss.ss_family == PF_LOCAL)
	_einfo.is_unix_socket = true;
    else if (ss.ss_family != PF_INET)
	return false;

    // If matches, need to set the fd nonblocking for the poll loop to work
    return !make_fd_nonblocking (fd);
}

void Extern::enable_credentials_passing (bool enable)
{
    if (_sockfd >= 0 && _einfo.is_unix_socket)
	if (0 > socket_enable_credentials_passing (_sockfd, enable))
	    return error_libc ("setsockopt(SO_PASSCRED)");
}

//}}}-------------------------------------------------------------------
//{{{ Extern::COM

void Extern::COM_error (const string_view& errmsg)
{
    // errors occuring on in the Extern Msger on the other side of the socket
    error ("%s", errmsg.c_str());	// report it on this side
}

void Extern::COM_export (string elist)
{
    // Other side of the socket listing exported interfaces as a comma-separated list
    _einfo.imported.clear();
    foreach (ei, elist) {
	auto eic = elist.find (',', ei);
	if (!eic)
	    eic = elist.end();
	*eic = 0;
	auto iid = App::interface_by_name (ei, eic+1-ei);
	if (iid)	// _einfo.imported only contains interfaces supported by this App
	    _einfo.imported.push_back (iid);
	ei = eic;
    }
    _reply.connected (msger_id());
}

void Extern::COM_delete (void)
{
    // This happens when the Extern Msger on the other side of the socket dies
    set_unused();
}

//}}}-------------------------------------------------------------------
//{{{ Extern::Timer

void Extern::Timer_timer (fd_t)
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
bool Extern::write_outgoing (void)
{
    // write all queued messages
    while (!_outq.empty()) {
	// Build sendmsg header
	msghdr mh = {};

	// Add fd if being passed
	int passedfd = _outq.front().passed_fd();
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
	enum { MAX_MSGS_PER_SEND = 32 };
	auto maxnm = min (_outq.size(), MAX_MSGS_PER_SEND);
	while (nm < maxnm && !_outq[nm].has_fd())
	    ++nm;

	// Create iovecs for output
	iovec iov [2*MAX_MSGS_PER_SEND];
	mh.msg_iov = iov;
	mh.msg_iovlen = 2*nm;	// two iovecs per message, header and body
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

void Extern::read_incoming (void)
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
	char cmsgbuf [CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(ucred))] = {};

	// Build struct for recvmsg
	msghdr mh = {};
	mh.msg_iov = iov;
	mh.msg_iovlen = 2 + (_bread >= sizeof(fh));	// read another header only when already have one
	mh.msg_control = cmsgbuf;
	mh.msg_controllen = sizeof(cmsgbuf);

	// Receive some data
	if (auto rmr = recvmsg (_sockfd, &mh, 0); rmr <= 0 || (mh.msg_flags & (MSG_CTRUNC|MSG_TRUNC))) {
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
	        if (cmsg->cmsg_len != CMSG_LEN(stream_sizeof(_einfo.creds)))
		    error ("invalid socket credentials");
		else {
		    istream (CMSG_DATA(cmsg), stream_sizeof(_einfo.creds)) >> _einfo.creds;
		    enable_credentials_passing (false);	// Credentials only need to be received once
		    DEBUG_PRINTF ("[X] Received credentials: pid=%u,uid=%u,gid=%u\n", _einfo.creds.pid, _einfo.creds.uid, _einfo.creds.gid);
		}
	    } else if (cmsg->cmsg_type == SCM_RIGHTS) {
		istream cis (CMSG_DATA(cmsg), cmsg->cmsg_len - CMSG_LEN(0));
		while (cis.remaining() >= sizeof(int)) {
		    if (_infd >= 0) {
			DEBUG_PRINTF ("[XE] Closing extra fd %d\n", _infd);
			close (_infd);
			error ("multiple file descriptors in one message");
		    }
		    _infd = cis.read<int>();
		}
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

bool Extern::accept_incoming_message (void)
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
	    DEBUG_PRINTF ("[XE] Incoming message requests unexported interface %s\n", interface_of_method (method));
	    return false;
	}
	// Verify that the other side sets extid correctly
	if (bool(_einfo.side) ^ (_inmsg.extid() < extid_ServerBase)) {
	    DEBUG_PRINTF ("[XE] Extern connection peer allocates incorrect extids\n");
	    return false;
	}
	DEBUG_PRINTF ("[X] Creating new extid link %hu with interface %s\n", _inmsg.extid(), interface_of_method (method));
	rp = &_relays.emplace_back (msger_id(), _inmsg.extid());
	//
	// Create a COMRelay as the destination. It will then create the
	// actual server Msger using the interface in the message.
	rp->relay.create_dest_for (PCOM::interface());
    }

    // Create local message from ExtMsg and forward it to the COMRelay
    rp->relay.forward_msg (method, _inmsg.move_body(), _inmsg.fd_offset(), _inmsg.extid());
    return true;
}
//}}}2
//}}}-------------------------------------------------------------------
//{{{ COMRelay

COMRelay::COMRelay (const Msg::Link& l)
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
,_localp (l.dest, _pExtern ? Proxy::allocate_id(l.dest) : l.src)
//
// Extid will be determined when the connection interface is known
,_extid()
{
}

COMRelay::~COMRelay (void)
{
    // The relay is destroyed when:
    // 1. The local Msger is destroyed. COM delete message is sent to the
    //    remote side as notification.
    // 2. The remote object is destroyed. The relay is marked unused in
    //    COMRelay_COM_delete and the extern pointer is reset to prevent
    //    further messages to remote object. Here, no message is sent.
    // 3. The Extern object is destroyed. pExtern is reset in the dtor of
    //    RelayProxy in the Extern object, calling COMRelay on_msger_destroyed.
    if (_pExtern && _extid)
	_pExtern->queue_outgoing (PCOM::delete_msg(), _extid);
    COM_delete();
    _pExtern = nullptr;
    _extid = 0;
}

bool COMRelay::dispatch (Msg& msg)
{
    // Broadcast messages are never exported or imported
    if (msg.dest() == mrid_Broadcast)
	return true;

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

bool COMRelay::on_error (mrid_t eid, const string& errmsg)
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

void COMRelay::on_msger_destroyed (mrid_t id)
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

void COMRelay::COM_error (const string_view& errmsg)
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

void COMRelay::COM_export (const string_view&)
{
    // Relays never receive this message
}

void COMRelay::COM_delete (void)
{
    // COM_delete indicates that the remote object has been destroyed.
    if (_pExtern)
	_pExtern->unregister_relay (this);
    _pExtern = nullptr;	// No further messages are to be sent.
    _extid = 0;
    set_unused();	// The relay and local object are to be destroyed.
}

//}}}-------------------------------------------------------------------
//{{{ PExternServer

PExternServer::~PExternServer (void)
{
    if (!_sockname.empty())
	unlink (_sockname.c_str());
}

/// Socket activation
auto PExternServer::activate (const iid_t* eifaces) -> fd_t
{
    // First check if socket activation is enabled
    if (auto e = getenv("LISTEN_PID"); !e || getpid() != pid_t(atoi(e)))
	return 0;	// normal return values will always be > SD_LISTEN_FDS_START

    // Not having LISTEN_FDS or having more than one are errors
    // Multiple sockets must be handled with an array of PExternServers
    if (auto e = getenv("LISTEN_FDS"); !e || 1 != atoi(e)) {
	errno = EBADF;
	return -1;
    }
    const fd_t fd = SD_LISTEN_FDS_START+0;

    // Passed in sockets are either already accepted, when named "connection"
    if (auto e = getenv("LISTEN_FDNAMES"); e && 0 == strcmp(e, "connection"))
	accept (fd, eifaces);
    else // or they are to be listened on
	listen (fd, eifaces);
    return fd;
}

/// Create server socket bound to the given address
auto PExternServer::bind (const sockaddr* addr, socklen_t addrlen, const iid_t* eifaces) -> fd_t
{
    DEBUG_PRINTF ("[X] Creating server socket %s\n", debug_socket_name(addr));
    auto fd = socket (addr->sa_family, SOCK_STREAM| SOCK_NONBLOCK| SOCK_CLOEXEC, 0);
    if (fd < 0)
	return fd;
    if (0 > ::bind (fd, addr, addrlen) && errno != EINPROGRESS) {
	DEBUG_PRINTF ("[E] bind failed: %s\n", strerror(errno));
	::close (fd);
	return -1;
    }
    if (0 > ::listen (fd, min (SOMAXCONN, 64))) {
	DEBUG_PRINTF ("[E] listen failed: %s\n", strerror(errno));
	::close (fd);
	return -1;
    }
    if (addr->sa_family == PF_LOCAL) {
	auto sockpath = pointer_cast<sockaddr_un>(addr)->sun_path;
	_sockname = sockpath;
	if (sockpath[0])
	    chmod (sockpath, DEFFILEMODE);
    }
    listen (fd, eifaces, WhenEmpty::Remain);
    return fd;
}

/// Create local socket with given path
auto PExternServer::bind_local (const char* path, const iid_t* eifaces) -> fd_t
{
    sockaddr_un addr;
    auto addrlen = create_sockaddr_local (&addr, path);
    if (addrlen < 0)
	return addrlen;
    return bind (pointer_cast<sockaddr>(&addr), addrlen, eifaces);
}

/// Create local socket of the given name in the system standard location for such
auto PExternServer::bind_system_local (const char* sockname, const iid_t* eifaces) -> fd_t
{
    sockaddr_un addr;
    auto addrlen = create_sockaddr_system_local (&addr, sockname);
    if (addrlen < 0)
	return addrlen;
    return bind (pointer_cast<sockaddr>(&addr), addrlen, eifaces);
}

/// Create local socket of the given name in the user standard location for such
auto PExternServer::bind_user_local (const char* sockname, const iid_t* eifaces) -> fd_t
{
    sockaddr_un addr;
    auto addrlen = create_sockaddr_user_local (&addr, sockname);
    if (addrlen < 0)
	return addrlen;
    return bind (pointer_cast<sockaddr>(&addr), addrlen, eifaces);
}

//}}}-------------------------------------------------------------------
//{{{ ExternServer

bool ExternServer::on_error (mrid_t eid, const string& errmsg [[maybe_unused]])
{
    if (_timer.dest() == eid || msger_id() == eid)
	return false;	// errors in listened socket are fatal
    // error in accepted socket. Handle by logging the error and removing record in ObjectDestroyed.
    DEBUG_PRINTF ("[X] Client connection error: %s", errmsg.c_str());
    return true;
}

void ExternServer::on_msger_destroyed (mrid_t mid)
{
    DEBUG_PRINTF ("[X] Client connection %hu dropped\n", mid);
    remove_if (_conns, [&](const auto& e) { return e.dest() == mid; });
    if (_conns.empty() && !flag (f_ListenWhenEmpty))
	set_unused();
}

bool ExternServer::dispatch (Msg& msg)
{
    return PTimer::Reply::dispatch (this, msg)
	|| PExternServer::dispatch (this, msg)
	|| PExtern::Reply::dispatch (this, msg)
	|| Msger::dispatch (msg);
}

void ExternServer::Timer_timer (PTimer::fd_t)
{
    for (int cfd; 0 <= (cfd = accept (_sockfd, nullptr, nullptr));)
	ExternServer_accept (cfd, _eifaces);
    if (errno == EAGAIN) {
	DEBUG_PRINTF ("[X] Resuming wait on fd %d\n", _sockfd);
	_timer.wait_read (_sockfd);
    } else {
	DEBUG_PRINTF ("[X] Accept failed with error %s\n", strerror(errno));
	error_libc ("accept");
    }
}

void ExternServer::ExternServer_listen (int fd, const iid_t* eifaces, PExternServer::WhenEmpty closeWhenEmpty)
{
    assert (_sockfd == -1 && "each ExternServer instance can only listen to one socket");
    if (make_fd_nonblocking (fd))
	return error_libc ("make_fd_nonblocking");
    _sockfd = fd;
    _eifaces = eifaces;
    set_flag (f_ListenWhenEmpty, !bool(closeWhenEmpty));
    Timer_timer (_sockfd);
}

void ExternServer::ExternServer_accept (int fd, const iid_t* eifaces)
{
    DEBUG_PRINTF ("[X] Client connection accepted on fd %d\n", fd);
    set_unused (false);
    _conns.emplace_back (msger_id()).open (fd, eifaces);
}

void ExternServer::ExternServer_close (void)
{
    set_unused();
    _timer.stop();
}

void ExternServer::Extern_connected (const Extern::Info* einfo)
{
    _reply.connected (einfo ? einfo->extern_id : 0);
}

} // namespace cwiclo
//}}}-------------------------------------------------------------------
