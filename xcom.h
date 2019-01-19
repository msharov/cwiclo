// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "app.h"
#include <sys/socket.h>
#include <netinet/in.h>

//{{{ COM --------------------------------------------------------------
namespace cwiclo {

class PCOM : public Proxy {
    DECLARE_INTERFACE (COM, (Error,"s")(Export,"s")(Delete,""))
public:
    constexpr	PCOM (mrid_t src, mrid_t dest)	: Proxy (src, dest) {}
		~PCOM (void) noexcept		{ FreeId(); }
    void	Error (const string& errmsg)	{ Send (M_Error(), errmsg); }
    void	Export (const string& elist)	{ Send (M_Export(), elist); }
    void	Delete (void)			{ Send (M_Delete()); }
    void	Forward (Msg&& msg)		{ Proxy::Forward (move(msg)); }
  static string	StringFromInterfaceList (const iid_t* elist) noexcept;
    static Msg	ErrorMsg (mrid_t extid, const string& errmsg) noexcept;
    static Msg	ExportMsg (mrid_t extid, const string& elstr) noexcept;
    static auto	ExportMsg (mrid_t extid, const iid_t* elist) noexcept
							{ return ExportMsg (extid, StringFromInterfaceList (elist)); }
    static auto	DeleteMsg (mrid_t extid) noexcept	{ return Msg (Msg::Link{}, PCOM::M_Delete(), 0, extid); }
    template <typename O>
    inline static bool Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() == M_Error())
	    o->COM_Error (lstring_from_const_stream (msg.Read()));
	else if (msg.Method() == M_Export())
	    o->COM_Export (lstring_from_const_stream (msg.Read()));
	else if (msg.Method() == M_Delete())
	    o->COM_Delete ();
	else
	    return false;
	return true;
    }
};

//}}}-------------------------------------------------------------------
//{{{ PExtern

class PExtern : public Proxy {
    DECLARE_INTERFACE (Extern, (Open,"xib")(Close,""))
public:
    using fd_t = PTimer::fd_t;
    enum class SocketSide : bool { Client, Server };
public:
    constexpr	PExtern (mrid_t caller)	: Proxy(caller) {}
		~PExtern (void)		{ FreeId(); }
    void	Close (void)		{ Send (M_Close()); }
    void	Open (fd_t fd, const iid_t* eifaces, SocketSide side = SocketSide::Server)
		    { Send (M_Open(), eifaces, fd, side); }
    void	Open (fd_t fd)		{ Open (fd, nullptr, SocketSide::Client); }
    fd_t	Connect (const sockaddr* addr, socklen_t addrlen) noexcept;
    fd_t	ConnectIP4 (in_addr_t ip, in_port_t port) noexcept;
    fd_t	ConnectIP6 (in6_addr ip, in_port_t port) noexcept;
    fd_t	ConnectLocal (const char* path) noexcept;
    fd_t	ConnectLocalIP4 (in_port_t port) noexcept;
    fd_t	ConnectLocalIP6 (in_port_t port) noexcept;
    fd_t	ConnectSystemLocal (const char* sockname) noexcept;
    fd_t	ConnectUserLocal (const char* sockname) noexcept;
    fd_t	LaunchPipe (const char* exe, const char* arg) noexcept;
    template <typename O>
    inline static bool Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() == M_Open()) {
	    auto is = msg.Read();
	    auto eifaces = is.readv<const iid_t*>();
	    auto fd = is.readv<fd_t>();
	    auto side = is.readv<SocketSide>();
	    o->Extern_Open (fd, eifaces, side);
	} else if (msg.Method() == M_Close())
	    o->Extern_Close();
	else
	    return false;
	return true;
    }
};

//}}}-------------------------------------------------------------------
//{{{ ExternInfo

struct ExternInfo {
    using SocketSide = PExtern::SocketSide;
    vector<iid_t>	imported;
    const iid_t*	exported;
    struct ucred	creds;
    mrid_t		oid;
    SocketSide		side;
    bool		isUnixSocket;
public:
    auto IsImporting (iid_t iid) const
	{ return linear_search (imported, iid); }
    bool IsExporting (iid_t iid) const {
	for (auto e = exported; e && *e; ++e)
	    if (*e == iid)
		return true;
	return false;
    }
};

//}}}-------------------------------------------------------------------
//{{{ PExternR

class PExternR : public ProxyR {
    DECLARE_INTERFACE (ExternR, (Connected,"x"))
public:
    constexpr	PExternR (const Msg::Link& l)		: ProxyR(l) {}
    void	Connected (const ExternInfo* einfo)	{ Send (M_Connected(), einfo); }
    template <typename O>
    inline static bool Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() != M_Connected())
	    return false;
	o->ExternR_Connected (msg.Read().readv<const ExternInfo*>());
	return true;
    }

};

//}}}-------------------------------------------------------------------
//{{{ COMRelay

class Extern;

class COMRelay : public Msger {
public:
    explicit		COMRelay (const Msg::Link& l) noexcept;
			~COMRelay (void) noexcept override;
    bool		Dispatch (Msg& msg) noexcept override;
    bool		OnError (mrid_t eid, const string& errmsg) noexcept override;
    void		OnMsgerDestroyed (mrid_t id) noexcept override;
    inline void		COM_Error (const lstring& errmsg) noexcept;
    inline void		COM_Export (const lstring& elist) noexcept;
    inline void		COM_Delete (void) noexcept;
private:
    Extern*	_pExtern;	// Outgoing connection object
    PCOM	_localp;	// Proxy to the local object
    mrid_t	_extid;		// Extern link id
};

//}}}-------------------------------------------------------------------
//{{{ Extern

class Extern : public Msger {
public:
    using fd_t = PExtern::fd_t;
protected:
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
	// and mrid + halfrange on the server
	// (32000 is 0x7d00, mostly round number in both bases; helpful in the debugger)
	extid_ServerBase = extid_ClientBase+32000,
	extid_ClientLast = extid_ServerBase-1,
	extid_ServerLast = extid_ServerBase+(extid_ClientLast-extid_ClientBase)
    };
public:
    explicit		Extern (const Msg::Link& l) noexcept;
			~Extern (void) noexcept override;
    auto&		Info (void) const	{ return _einfo; }
    bool		Dispatch (Msg& msg) noexcept override;
    void		QueueOutgoing (Msg&& msg) noexcept;
    static Extern*	LookupById (mrid_t id) noexcept;
    static Extern*	LookupByImported (iid_t id) noexcept;
    static Extern*	LookupByRelayId (mrid_t rid) noexcept;
    mrid_t		RegisterRelay (const COMRelay* relay) noexcept;
    void		UnregisterRelay (const COMRelay* relay) noexcept;
    inline void		Extern_Open (fd_t fd, const iid_t* eifaces, PExtern::SocketSide side) noexcept;
    void		Extern_Close (void) noexcept;
    inline void		COM_Error (const lstring& errmsg) noexcept;
    inline void		COM_Export (string elist) noexcept;
    inline void		COM_Delete (void) noexcept;
    void		TimerR_Timer (fd_t fd) noexcept;
private:
    //{{{2 ExtMsg ------------------------------------------------------
    // Message formatted for reading/writing to socket
    class ExtMsg {
    public:
	struct alignas(8) Header {
	    uint32_t	sz;		// Message body size, aligned to c_MsgAlignment
	    uint16_t	extid;		// Destination node mrid
	    uint8_t	fdoffset;	// Offset to file descriptor in message body, if passing
	    uint8_t	hsz;		// Full size of header
	};
	enum {
	    c_MinHeaderSize = Align (sizeof(Header)+sizeof("i\0m\0"), Msg::Alignment::Header),
	    c_MaxHeaderSize = UINT8_MAX-sizeof(Header),
	    c_MaxBodySize = (1<<24)-1
	};
    public:
	constexpr		ExtMsg (void)		: _body(),_h{},_hbuf{} {}
	inline			ExtMsg (Msg&& msg) noexcept;
	constexpr streamsize	HeaderSize (void) const	{ return _h.hsz; }
	constexpr auto&		GetHeader (void) const	{ return _h; }
	constexpr streamsize	BodySize (void) const	{ return _h.sz; }
	constexpr auto		Extid (void) const	{ return _h.extid; }
	constexpr auto		FdOffset (void) const	{ return _h.fdoffset; }
	constexpr streamsize	Size (void) const	{ return BodySize() + HeaderSize(); }
	constexpr bool		HasFd (void) const	{ return FdOffset() != Msg::NoFdIncluded; }
	void			SetHeader (const Header& h)	{ _h = h; }
	void			AllocateBody (streamsize sz)	{ _body.allocate (sz); }
	void			TrimBody (streamsize sz)	{ _body.shrink (sz); }
	constexpr auto&&	MoveBody (void)		{ return move(_body); }
	void			SetPassedFd (fd_t fd)	{ assert (HasFd()); ostream os (_body.iat(_h.fdoffset), sizeof(fd)); os << fd; }
	fd_t			PassedFd (void) const noexcept;
	void			WriteIOVecs (iovec* iov, streamsize bw) noexcept;
	constexpr auto		Read (void) const	{ return istream (_body.data(), _body.size()); }
	methodid_t		ParseMethod (void) const noexcept;
	inline void		DebugDump (void) const noexcept;
    private:
	constexpr auto		HeaderPtr (void) const	{ auto hp = _hbuf; return hp-sizeof(_h); }
	auto			HeaderPtr (void)	{ return UNCONST_MEMBER_FN (HeaderPtr,); }
	uint8_t			WriteHeaderStrings (methodid_t method) noexcept;
    private:
	Msg::Body		_body;
	Header			_h;
	char			_hbuf [c_MaxHeaderSize];
    };
    //}}}2--------------------------------------------------------------
    //{{{2 RelayProxy
    struct RelayProxy {
	const COMRelay*	pRelay;
	PCOM		relay;
	mrid_t		extid;
    public:
	RelayProxy (mrid_t src, mrid_t dest, mrid_t eid)
	    : pRelay(), relay(src,dest), extid(eid) {}
	RelayProxy (const RelayProxy&) = delete;
	void operator= (const RelayProxy&) = delete;
    };
    //}}}2--------------------------------------------------------------
private:
    constexpr mrid_t	CreateExtidFromRelayId (mrid_t id) const noexcept
			    { return id + ((_einfo.side == ExternInfo::SocketSide::Client) ? extid_ClientBase : extid_ServerBase); }
    static auto&	ExternList (void) noexcept
			    { static vector<Extern*> s_ExternList; return s_ExternList; }
    RelayProxy*		RelayProxyByExtid (mrid_t extid) noexcept;
    RelayProxy*		RelayProxyById (mrid_t id) noexcept;
    bool		WriteOutgoing (void) noexcept;
    void		ReadIncoming (void) noexcept;
    inline bool		AcceptIncomingMessage (void) noexcept;
    inline bool		AttachToSocket (fd_t fd) noexcept;
    void		EnableCredentialsPassing (bool enable) noexcept;
private:
    fd_t		_sockfd;
    PTimer		_timer;
    PExternR		_reply;
    streamsize		_bwritten;
    vector<ExtMsg>	_outq;		// messages queued for export
    vector<RelayProxy>	_relays;
    ExternInfo		_einfo;
    streamsize		_bread;
    ExtMsg		_inmsg;		// currently incoming message
    fd_t		_infd;
};

#define REGISTER_EXTERNS\
    REGISTER_MSGER (Extern, Extern)\
    REGISTER_MSGER (COM, COMRelay)\
    REGISTER_MSGER (Timer, App::Timer)

#define REGISTER_EXTERN_MSGER(iface)\
    REGISTER_MSGER (iface, COMRelay)

//}}}-------------------------------------------------------------------
//{{{ PExternServer

class PExternServer : public Proxy {
    DECLARE_INTERFACE (ExternServer, (Open,"xib")(Close,""))
public:
    using fd_t = PExtern::fd_t;
    enum class WhenEmpty : bool { Remain, Close };
public:
		PExternServer (mrid_t caller)	: Proxy(caller),_sockname() {}
		~PExternServer (void) noexcept;
    void	Close (void)			{ Send (M_Close()); }
    void	Open (fd_t fd, const iid_t* eifaces, WhenEmpty closeWhenEmpty = WhenEmpty::Close)
		    { Send (M_Open(), eifaces, fd, closeWhenEmpty); }
    fd_t	Bind (const sockaddr* addr, socklen_t addrlen, const iid_t* eifaces) noexcept NONNULL();
    fd_t	BindLocal (const char* path, const iid_t* eifaces) noexcept NONNULL();
    fd_t	BindUserLocal (const char* sockname, const iid_t* eifaces) noexcept NONNULL();
    fd_t	BindSystemLocal (const char* sockname, const iid_t* eifaces) noexcept NONNULL();
    fd_t	BindIP4 (in_addr_t ip, in_port_t port, const iid_t* eifaces) noexcept NONNULL();
    fd_t	BindLocalIP4 (in_port_t port, const iid_t* eifaces) noexcept NONNULL();
    fd_t	BindIP6 (in6_addr ip, in_port_t port, const iid_t* eifaces) noexcept NONNULL();
    fd_t	BindLocalIP6 (in_port_t port, const iid_t* eifaces) noexcept NONNULL();
    template <typename O>
    inline static bool Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() == M_Open()) {
	    auto is = msg.Read();
	    auto eifaces = is.readv<const iid_t*>();
	    auto fd = is.readv<fd_t>();
	    auto closeWhenEmpty = is.readv<WhenEmpty>();
	    o->ExternServer_Open (fd, eifaces, closeWhenEmpty);
	} else if (msg.Method() == M_Close())
	    o->ExternServer_Close();
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
    enum { f_CloseWhenEmpty = Msger::f_Last, f_Last };
public:
    using fd_t = PExternServer::fd_t;
public:
    explicit constexpr	ExternServer (const Msg::Link& l) noexcept;
    bool		OnError (mrid_t eid, const string& errmsg) noexcept override;
    void		OnMsgerDestroyed (mrid_t mid) noexcept override;
    bool		Dispatch (Msg& msg) noexcept override;
    inline void		TimerR_Timer (fd_t) noexcept;
    inline void		ExternServer_Open (fd_t fd, const iid_t* eifaces, PExternServer::WhenEmpty closeWhenEmpty) noexcept;
    inline void		ExternServer_Close (void) noexcept;
    inline void		ExternR_Connected (const ExternInfo* einfo) noexcept;
private:
    vector<PExtern>	_conns;
    const iid_t*	_eifaces;
    PTimer		_timer;
    PExternR		_reply;
    fd_t		_sockfd;
};

constexpr ExternServer::ExternServer (const Msg::Link& l) noexcept
: Msger(l)
,_conns()
,_eifaces()
,_timer (l.dest)
,_reply (l)
,_sockfd (-1)
{
}

} // namespace cwiclo
//}}}-------------------------------------------------------------------
