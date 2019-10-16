// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "app.h"
#include <sys/socket.h>
#include <netinet/in.h>

//{{{ COM --------------------------------------------------------------
namespace cwiclo {

class PCOM : public Proxy {
    DECLARE_INTERFACE (COM, (error,"s")(export,"s")(delete,""))
public:
    constexpr		PCOM (mrid_t src, mrid_t dest)	: Proxy (src, dest) {}
			~PCOM (void)			{ }
    void		error (const string& errmsg)	{ send (m_error(), errmsg); }
    void		export_ (const string& elist)	{ send (m_export(), elist); }
    void		delete_ (void)			{ send (m_delete()); }
    void		forward_msg (Msg&& msg)		{ Proxy::forward_msg (move(msg)); }
    static string	string_from_interface_list (const iid_t* elist);
    static Msg		error_msg (const string& errmsg);
    static Msg		export_msg (const string& elstr);
    static auto		export_msg (const iid_t* elist)
			    { return export_msg (string_from_interface_list (elist)); }
    static auto		delete_msg (void)
			    { return Msg (Msg::Link{}, m_delete(), 0, Msg::NoFdIncluded); }
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() == m_error())
	    o->COM_error (msg.read().read<string_view>());
	else if (msg.method() == m_export())
	    o->COM_export (msg.read().read<string_view>());
	else if (msg.method() == m_delete())
	    o->COM_delete ();
	else
	    return false;
	return true;
    }
};

//}}}-------------------------------------------------------------------
//{{{ COMRelay

class Extern;

class COMRelay : public Msger {
public:
    explicit		COMRelay (const Msg::Link& l);
			~COMRelay (void) override;
    bool		dispatch (Msg& msg) override;
    bool		on_error (mrid_t eid, const string& errmsg) override;
    void		on_msger_destroyed (mrid_t id) override;
    inline void		COM_error (const string_view& errmsg);
    inline void		COM_export (const string_view& elist);
    void		COM_delete (void);
private:
    Extern*	_pExtern;	// Outgoing connection object
    PCOM	_localp;	// Proxy to the local object
    mrid_t	_extid;		// Extern link id
};

//}}}-------------------------------------------------------------------
//{{{ PExtern

class PExtern : public Proxy {
    DECLARE_INTERFACE (Extern, (open,"xib")(close,""))
public:
    using fd_t = PTimer::fd_t;
    //{{{2 Info
    enum class SocketSide : bool { Client, Server };
    struct Info {
	vector<iid_t>	imported;
	const iid_t*	exported;
	struct ucred	creds;
	mrid_t		oid;
	SocketSide	side;
	bool		is_unix_socket;
    public:
	constexpr auto is_importing (iid_t iid) const
	    { return linear_search (imported, iid); }
	constexpr bool is_exporting (iid_t iid) const {
	    for (auto e = exported; e && *e; ++e)
		if (*e == iid)
		    return true;
	    return false;
	}
    };
    //}}}2
public:
    constexpr	PExtern (mrid_t caller)	: Proxy(caller) {}
		~PExtern (void)		{ free_id(); }
    void	close (void)		{ send (m_close()); }
    void	open (fd_t fd, const iid_t* eifaces, SocketSide side = SocketSide::Server)
		    { send (m_open(), eifaces, fd, side); }
    void	open (fd_t fd)		{ open (fd, nullptr, SocketSide::Client); }
    fd_t	connect (const sockaddr* addr, socklen_t addrlen);
    fd_t	connect_ip4 (in_addr_t ip, in_port_t port);
    fd_t	connect_ip6 (in6_addr ip, in_port_t port);
    fd_t	connect_local (const char* path);
    fd_t	connect_local_ip4 (in_port_t port);
    fd_t	connect_local_ip6 (in_port_t port);
    fd_t	connect_system_local (const char* sockname);
    fd_t	connect_user_local (const char* sockname);
    fd_t	launch_pipe (const char* exe, const char* arg);
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() == m_open()) {
	    auto is = msg.read();
	    auto eifaces = is.read<const iid_t*>();
	    auto fd = is.read<fd_t>();
	    auto side = is.read<SocketSide>();
	    o->Extern_open (fd, eifaces, side);
	} else if (msg.method() == m_close())
	    o->Extern_close();
	else
	    return false;
	return true;
    }
};

//}}}-------------------------------------------------------------------
//{{{ PExternR

class PExternR : public ProxyR {
    DECLARE_INTERFACE (ExternR, (connected,"x"))
public:
    constexpr	PExternR (const Msg::Link& l)		: ProxyR(l) {}
    void	connected (const PExtern::Info* einfo)	{ send (m_connected(), einfo); }
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() != m_connected())
	    return false;
	o->ExternR_connected (msg.read().read<const PExtern::Info*>());
	return true;
    }

};

//}}}-------------------------------------------------------------------
//{{{ Extern

class Extern : public Msger {
public:
    using fd_t = PExtern::fd_t;
    using Info = PExtern::Info;
public:
    explicit		Extern (const Msg::Link& l);
			~Extern (void) override;
    auto&		info (void) const	{ return _einfo; }
    bool		dispatch (Msg& msg) override;
    void		queue_outgoing (Msg&& msg, mrid_t extid)
			    { queue_outgoing (msg.method(), msg.move_body(), msg.fd_offset(), extid); }
    static Extern*	lookup_by_id (mrid_t id);
    static Extern*	lookup_by_imported (iid_t id);
    static Extern*	lookup_by_relay_id (mrid_t rid);
    mrid_t		register_relay (COMRelay* relay);
    void		unregister_relay (const COMRelay* relay);
    inline void		Extern_open (fd_t fd, const iid_t* eifaces, PExtern::SocketSide side);
    void		Extern_close (void);
    inline void		COM_error (const string_view& errmsg);
    inline void		COM_export (string elist);
    inline void		COM_delete (void);
    void		TimerR_timer (fd_t fd);
private:
    //{{{2 ExtMsg ------------------------------------------------------
    // Msg formatted for reading/writing to socket
    class ExtMsg {
    public:
	struct alignas(8) Header {
	    uint32_t	sz;		// Message body size, aligned to MsgAlignment
	    uint16_t	extid;		// Destination node mrid
	    uint8_t	fdoffset;	// Offset to file descriptor in message body, if passing
	    uint8_t	hsz;		// Full size of header
	};
	enum {
	    MinHeaderSize = ceilg (sizeof(Header)+sizeof("i\0m\0"), Msg::Alignment::Header),
	    MaxHeaderSize = UINT8_MAX-sizeof(Header),
	    MaxBodySize = Msg::MaxSize
	};
    public:
	constexpr		ExtMsg (void)		: _body(),_h{},_hbuf{} {}
				ExtMsg (methodid_t mid, Msg::Body&& body, Msg::fdoffset_t fdo, mrid_t extid);
				ExtMsg (const ExtMsg&) = delete;
	void			operator= (const ExtMsg&) = delete;
	constexpr auto&		header (void) const		{ return _h; }
	constexpr auto		extid (void) const		{ return _h.extid; }
	constexpr auto		fd_offset (void) const		{ return _h.fdoffset; }
	constexpr streamsize	header_size (void) const	{ return _h.hsz; }
	constexpr streamsize	body_size (void) const		{ return _h.sz; }
	constexpr streamsize	size (void) const		{ return body_size() + header_size(); }
	constexpr bool		has_fd (void) const		{ return fd_offset() != Msg::NoFdIncluded; }
	constexpr void		set_header (const Header& h)	{ _h = h; }
	void			allocate_body (streamsize sz)	{ _body.allocate (sz); }
	constexpr void		trim_body (streamsize sz)	{ _body.shrink (sz); }
	constexpr auto&&	move_body (void)		{ return move(_body); }
	constexpr void		set_passed_fd (fd_t fd)		{ assert (has_fd()); ostream(_body.iat(_h.fdoffset), sizeof(fd)) << fd; }
	constexpr fd_t		passed_fd (void) const { return has_fd() ? istream(_body.iat(_h.fdoffset), sizeof(fd_t)).read<fd_t>() : -1; }
	void			write_iovecs (iovec* iov, streamsize bw);
	constexpr auto		read (void) const		{ return istream (_body.data(), _body.size()); }
	methodid_t		parse_method (void) const;
	inline void		debug_dump (void) const;
    private:
	constexpr auto		header_ptr (void) const		{ return begin(_hbuf)-sizeof(_h); }
	constexpr auto		header_ptr (void)		{ return UNCONST_MEMBER_FN (header_ptr,); }
	constexpr uint8_t	write_header_strings (methodid_t method) {
				    // _hbuf contains iface\0method\0signature\0, padded to Msg::Alignment::Header
				    auto iface = interface_of_method (method);
				    assert (ptrdiff_t(sizeof(_hbuf)) >= interface_name_size(iface)+method_next_offset(method)-2 && "the interface and method names for this message are too long to export");
				    ostream os (_hbuf);
				    os.write (iface, interface_name_size (iface));
				    os.write (method, method_next_offset(method)-2);
				    os.align (Msg::Alignment::Header);
				    return sizeof(_h) + os.ptr()-begin(_hbuf);
				}
    private:
	Msg::Body		_body;
	Header			_h;
	char			_hbuf [MaxHeaderSize];
    };
    //}}}2--------------------------------------------------------------
    //{{{2 RelayProxy
    struct RelayProxy {
	COMRelay*	pRelay;
	PCOM		relay;
	mrid_t		extid;
    public:
	constexpr RelayProxy (mrid_t src, mrid_t dest, mrid_t eid)
	    : pRelay(), relay(src,dest), extid(eid) {}
	~RelayProxy (void)	// Free ids for relays created by this Extern
	    { if (pRelay && pRelay->creator_id() == relay.src()) relay.free_id(); }
	RelayProxy (const RelayProxy&) = delete;
	void operator= (const RelayProxy&) = delete;
    };
    //}}}2--------------------------------------------------------------
    //{{{2 extid_ constants
    enum {
	// Each extern connection has two sides and each side must be able
	// to assign a unique extid to each Msger-Msger link across the socket.
	// Msger ids for COMRelays are unique for each process, and so can be
	// used as an extid on one side. The other side, however, needs another
	// address space. Here it is provided as extid_ServerBase. Extern
	// connection is bidirectional and has no functional difference between
	// sides, so selecting a server side is an arbitrary choice. By default,
	// the side that binds to the connection socket is the server and the
	// side that connects is the client.
	//
	extid_ClientBase,
	extid_COM = extid_ClientBase,
	extid_ClientLast = extid_ClientBase + mrid_Last,
	extid_ServerBase = extid_ClientLast + 1,
	extid_ServerLast = extid_ServerBase + mrid_Last
    };
    //}}}2--------------------------------------------------------------
private:
    void		queue_outgoing (methodid_t mid, Msg::Body&& body, Msg::fdoffset_t fdo, mrid_t extid);
    constexpr mrid_t	create_extid_from_relay_id (mrid_t id) const
			    { return id + ((_einfo.side == PExtern::SocketSide::Client) ? extid_ClientBase : extid_ServerBase); }
    static auto&	extern_list (void)
			    { static vector<Extern*> s_externs; return s_externs; }
    RelayProxy*		relay_proxy_by_extid (mrid_t extid);
    RelayProxy*		relay_proxy_by_id (mrid_t id);
    bool		write_outgoing (void);
    void		read_incoming (void);
    inline bool		accept_incoming_message (void);
    inline bool		attach_to_socket (fd_t fd);
    void		enable_credentials_passing (bool enable);
private:
    fd_t		_sockfd;
    PTimer		_timer;
    PExternR		_reply;
    streamsize		_bwritten;
    vector<ExtMsg>	_outq;		// messages queued for export
    vector<RelayProxy>	_relays;
    Info		_einfo;
    streamsize		_bread;
    ExtMsg		_inmsg;		// currently incoming message
    fd_t		_infd;
};

#define REGISTER_EXTERNS\
    REGISTER_MSGER (PExtern, Extern)\
    REGISTER_MSGER (PCOM, COMRelay)\
    REGISTER_MSGER (PTimer, App::Timer)

#define REGISTER_EXTERN_MSGER(proxy)\
    REGISTER_MSGER (proxy, COMRelay)

//}}}-------------------------------------------------------------------
//{{{ PExternServer

class PExternServer : public Proxy {
    DECLARE_INTERFACE (ExternServer, (open,"xib")(close,""))
public:
    using fd_t = PExtern::fd_t;
    enum class WhenEmpty : uint8_t { Remain, Close };
public:
    constexpr	PExternServer (mrid_t caller)	: Proxy(caller),_sockname() {}
		~PExternServer (void);
    void	close (void)			{ send (m_close()); }
    void	open (fd_t fd, const iid_t* eifaces, WhenEmpty whenempty = WhenEmpty::Close)
		    { send (m_open(), eifaces, fd, whenempty); }
    fd_t	bind (const sockaddr* addr, socklen_t addrlen, const iid_t* eifaces) NONNULL();
    fd_t	bind_local (const char* path, const iid_t* eifaces) NONNULL();
    fd_t	bind_user_local (const char* sockname, const iid_t* eifaces) NONNULL();
    fd_t	bind_system_local (const char* sockname, const iid_t* eifaces) NONNULL();
    fd_t	bind_ip4 (in_addr_t ip, in_port_t port, const iid_t* eifaces) NONNULL();
    fd_t	bind_local_ip4 (in_port_t port, const iid_t* eifaces) NONNULL();
    fd_t	bind_ip6 (in6_addr ip, in_port_t port, const iid_t* eifaces) NONNULL();
    fd_t	bind_local_ip6 (in_port_t port, const iid_t* eifaces) NONNULL();
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() == m_open()) {
	    auto is = msg.read();
	    auto eifaces = is.read<const iid_t*>();
	    auto fd = is.read<fd_t>();
	    auto closeWhenEmpty = is.read<WhenEmpty>();
	    o->ExternServer_open (fd, eifaces, closeWhenEmpty);
	} else if (msg.method() == m_close())
	    o->ExternServer_close();
	else
	    return false;
	return true;
    }
private:
    string	_sockname;
};

//}}}-------------------------------------------------------------------
//{{{ ExternServer

class ExternServer : public Msger {
public:
    using fd_t = PExternServer::fd_t;
    enum { f_CloseWhenEmpty = Msger::f_Last, f_Last };
public:
    explicit constexpr	ExternServer (const Msg::Link& l)
			    : Msger(l),_conns(),_eifaces(),_timer (l.dest),_reply (l),_sockfd (-1) {}
    bool		on_error (mrid_t eid, const string& errmsg) override;
    void		on_msger_destroyed (mrid_t mid) override;
    bool		dispatch (Msg& msg) override;
    inline void		TimerR_timer (fd_t);
    inline void		ExternServer_open (fd_t fd, const iid_t* eifaces, PExternServer::WhenEmpty whenempty);
    inline void		ExternServer_close (void);
    inline void		ExternR_connected (const Extern::Info* einfo);
private:
    vector<PExtern>	_conns;
    const iid_t*	_eifaces;
    PTimer		_timer;
    PExternR		_reply;
    fd_t		_sockfd;
};

} // namespace cwiclo
//}}}-------------------------------------------------------------------
