// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "app.h"

//{{{ COM --------------------------------------------------------------

struct sockaddr;
struct iovec;

namespace cwiclo {

class PCOM : public Proxy {
    DECLARE_INTERFACE (COM, (error,"s")(export,"s")(delete,""))
public:
    constexpr		PCOM (mrid_t src, mrid_t dest)	: Proxy (src, dest) {}
    explicit		PCOM (mrid_t src)		: Proxy (src) {}
			~PCOM (void)			{ }
    void		error (const string& errmsg) const	{ send (m_error(), errmsg); }
    void		export_ (const string& elist) const	{ send (m_export(), elist); }
    void		delete_ (void) const			{ send (m_delete()); }
			using Proxy::forward_msg;
    decltype(auto)	forward_msg (methodid_t imethod, Msg::Body&& body, Msg::fdoffset_t fdo, extid_t extid) const
			    { return Proxy::create_msg (imethod, move(body), fdo, extid); }
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
    extid_t	_extid;		// Extern link id
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
	struct Credentials {
	    pid_t	pid;
	    uid_t	uid;
	    #ifdef SCM_CREDS
		uid_t	euid; // BSD-only field, do not use
	    #endif
	    gid_t	gid;
	}		creds;
	mrid_t		extern_id;
	SocketSide	side;
	bool		is_unix_socket;
    public:
	constexpr auto is_importing (iid_t iid) const
	    { return find (imported, iid); }
	constexpr bool is_exporting (iid_t iid) const {
	    for (auto e = exported; e && *e; ++e)
		if (*e == iid)
		    return true;
	    return false;
	}
    };
    //}}}2
public:
    explicit	PExtern (mrid_t caller)	: Proxy(caller) {}
		~PExtern (void)		{ free_id(); }
    void	close (void) const	{ send (m_close()); }
    void	open (fd_t fd, const iid_t* eifaces, SocketSide side = SocketSide::Server) const
		    { send (m_open(), ios::ptr(eifaces), fd, side); }
    void	open (fd_t fd) const	{ open (fd, nullptr, SocketSide::Client); }
    fd_t	connect (const struct sockaddr* addr, socklen_t addrlen) const;
    fd_t	connect_local (const char* path) const;
    fd_t	connect_system_local (const char* sockname) const;
    fd_t	connect_user_local (const char* sockname) const;
    fd_t	launch_pipe (const char* exe, const char* arg = nullptr) const;
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() == m_open()) {
	    auto is = msg.read();
	    auto eifaces = is.read<ios::ptr<iid_t>>();
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
    DECLARE_INTERFACE (ExternR, (connected,"q"))
public:
    constexpr	PExternR (const Msg::Link& l) : ProxyR(l) {}
    void	connected (mrid_t extern_id) const
		    { send (m_connected(), extern_id); }
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg);
	// body below Extern because it calls Extern::lookup_by_id
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
    void		queue_outgoing (Msg&& msg, extid_t extid)
			    { queue_outgoing (msg.method(), msg.move_body(), msg.fd_offset(), extid); }
    static Extern*	lookup_by_id (mrid_t id);
    static Extern*	lookup_by_imported (iid_t id);
    static Extern*	lookup_by_relay_id (mrid_t rid);
    extid_t		register_relay (COMRelay* relay);
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
				ExtMsg (methodid_t mid, Msg::Body&& body, Msg::fdoffset_t fdo, extid_t extid);
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
	inline uint8_t		write_header_strings (methodid_t method);
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
	extid_t		extid;
    public:
	constexpr RelayProxy (mrid_t src, mrid_t dest, mrid_t eid)
	    : pRelay(), relay(src,dest), extid(eid) {}
	RelayProxy (mrid_t src, mrid_t eid)
	    : pRelay(), relay(src), extid(eid) {}
	~RelayProxy (void)	// Free ids for relays created by this Extern
	    { if (pRelay && pRelay->creator_id() == relay.src()) relay.free_id(); }
	RelayProxy (const RelayProxy&) = delete;
	void operator= (const RelayProxy&) = delete;
    };
    //}}}2--------------------------------------------------------------
private:
    void		queue_outgoing (methodid_t mid, Msg::Body&& body, Msg::fdoffset_t fdo, extid_t extid);
    constexpr extid_t	create_extid_from_relay_id (mrid_t id) const
			    { return id + ((_einfo.side == PExtern::SocketSide::Client) ? extid_ClientBase : extid_ServerBase); }
    static auto&	extern_list (void)
			    { static vector<Extern*> s_externs; return s_externs; }
    RelayProxy*		relay_proxy_by_extid (extid_t extid);
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

//----------------------------------------------------------------------

#define REGISTER_EXTERNS\
    REGISTER_MSGER (PExtern, Extern)\
    REGISTER_MSGER (PCOM, COMRelay)\
    REGISTER_MSGER (PTimer, App::Timer)

#define REGISTER_EXTERN_MSGER(proxy)\
    REGISTER_MSGER (proxy, COMRelay)

//----------------------------------------------------------------------

template <typename O>
constexpr bool PExternR::dispatch (O* o, const Msg& msg) { // static
    if (msg.method() != m_connected())
	return false;
    auto pextern = Extern::lookup_by_id (msg.read().read<mrid_t>());
    o->ExternR_connected (pextern ? &pextern->info() : nullptr);
    return true;
}

//}}}-------------------------------------------------------------------
//{{{ PExternServer

class PExternServer : public Proxy {
    DECLARE_INTERFACE (ExternServer, (listen,"xib")(accept,"xi")(close,""))
public:
    using fd_t = PExtern::fd_t;
    enum class WhenEmpty : uint8_t { Remain, Close };
public:
    explicit	PExternServer (mrid_t caller)	: Proxy(caller),_sockname() {}
		~PExternServer (void);
    void	close (void) const		{ send (m_close()); }
    void	listen (fd_t fd, const iid_t* eifaces, WhenEmpty whenempty = WhenEmpty::Close) const
		    { send (m_listen(), ios::ptr(eifaces), fd, whenempty); }
    void	accept (fd_t fd, const iid_t* eifaces) const
		    { send (m_accept(), ios::ptr(eifaces), fd); }

    fd_t	bind (const sockaddr* addr, socklen_t addrlen, const iid_t* eifaces) NONNULL();
    fd_t	bind_local (const char* path, const iid_t* eifaces) NONNULL();
    fd_t	bind_user_local (const char* sockname, const iid_t* eifaces) NONNULL();
    fd_t	bind_system_local (const char* sockname, const iid_t* eifaces) NONNULL();

    fd_t	activate (const iid_t* eifaces) NONNULL();
    fd_t	activate_local (const char* path, const iid_t* eifaces)
		    { auto fd = activate (eifaces); return fd ? fd : bind_local (path, eifaces); }
    fd_t	activate_user_local (const char* sockname, const iid_t* eifaces)
		    { auto fd = activate (eifaces); return fd ? fd : bind_user_local (sockname, eifaces); }
    fd_t	activate_system_local (const char* sockname, const iid_t* eifaces)
		    { auto fd = activate (eifaces); return fd ? fd : bind_system_local (sockname, eifaces); }

    template <typename O>
    inline static bool dispatch (O* o, const Msg& msg) {
	if (msg.method() == m_listen()) {
	    auto is = msg.read();
	    auto eifaces = is.read<ios::ptr<iid_t>>();
	    auto fd = is.read<fd_t>();
	    auto closeWhenEmpty = is.read<WhenEmpty>();
	    o->ExternServer_listen (fd, eifaces, closeWhenEmpty);
	} else if (msg.method() == m_accept()) {
	    auto is = msg.read();
	    auto eifaces = is.read<ios::ptr<iid_t>>();
	    auto fd = is.read<fd_t>();
	    o->ExternServer_accept (fd, eifaces);
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
    enum { f_ListenWhenEmpty = Msger::f_Last, f_Last };
public:
    explicit		ExternServer (const Msg::Link& l)
			    : Msger(l),_conns(),_eifaces(),_timer (l.dest),_reply (l),_sockfd (-1) {}
    bool		on_error (mrid_t eid, const string& errmsg) override;
    void		on_msger_destroyed (mrid_t mid) override;
    bool		dispatch (Msg& msg) override;
private:
			friend class PTimerR;
    inline void		TimerR_timer (fd_t);
			friend class PExternServer;
    inline void		ExternServer_listen (fd_t fd, const iid_t* eifaces, PExternServer::WhenEmpty whenempty);
    inline void		ExternServer_accept (int fd, const iid_t* eifaces);
    inline void		ExternServer_close (void);
			friend class PExternR;
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
