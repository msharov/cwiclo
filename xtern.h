// This file is part of the cwiclo project
//
// Copyright (c) 2020 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "xcom.h"
#include "app.h"

//{{{ COMRelay

struct iovec;

namespace cwiclo {

class Extern;

class COMRelay : public Msger {
    IMPLEMENT_INTERFACES_I_M (Msger, (ICOM),)
public:
    explicit		COMRelay (Msg::Link l);
			~COMRelay (void) override;
    bool		dispatch (Msg& msg) override;
    bool		on_error (mrid_t eid, const string& errmsg) override;
    void		on_msger_destroyed (mrid_t id) override;
    inline void		COM_error (const string_view& errmsg);
    inline void		COM_export (const string_view& elist);
    void		COM_delete (void);
private:
    Extern*	_pExtern;	// Outgoing connection object
    ICOM	_localp;	// Interface to the local object
    extid_t	_extid;		// Extern link id
};

//}}}-------------------------------------------------------------------
//{{{ Extern

class Extern : public Msger {
    IMPLEMENT_INTERFACES_I (Msger, (IExtern), (ITimer)(ICOM))
public:
    using Info = IExtern::Info;
public:
    explicit		Extern (Msg::Link l);
			~Extern (void) override;
    auto&		info (void) const	{ return _einfo; }
    void		queue_outgoing (Msg&& msg, extid_t extid);
    void		queue_pending (Msg&& msg) { _pending.emplace_back (move(msg)); }
    extid_t		register_relay (COMRelay* relay);
    void		unregister_relay (const COMRelay* relay);
    inline void		Extern_open (fd_t fd, const iid_t* eifaces, IExtern::SocketSide side);
    void		Extern_close (void);
    inline void		COM_error (const string_view& errmsg);
    inline void		COM_export (string elist);
    inline void		COM_delete (void);
    void		Timer_timer (fd_t fd);
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
	constexpr		ExtMsg (void)		: _body(),_h{},_hbuf{}{}
				ExtMsg (Msg&& msg, extid_t extid);
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
	void			allocate_body (streamsize sz)	{ _body.resize (sz); }
	constexpr void		trim_body (streamsize sz)	{ _body.shrink (sz); }
	constexpr auto&&	move_body (void)		{ return move(_body); }
	constexpr void		set_passed_fd (fd_t fd)		{ assert (has_fd()); ostream(_body.iat(_h.fdoffset), sizeof(fd)) << fd; }
	constexpr fd_t		passed_fd (void) const		{ return has_fd() ? istream(_body.iat(_h.fdoffset), sizeof(fd_t)).read<fd_t>() : -1; }
	void			write_iovecs (iovec* iov, streamsize bw);
	constexpr auto		read (void) const		{ return istream (_body.data(), _body.size()); }
	methodid_t		parse_method (void);
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
    //{{{2 PRelay
    struct PRelay {
	COMRelay*	pRelay;
	ICOM		relay;
	extid_t		extid;
    public:
	constexpr PRelay (mrid_t src, mrid_t dest, mrid_t eid)
	    : pRelay(), relay(src,dest), extid(eid) {}
	PRelay (mrid_t src, mrid_t eid)
	    : pRelay(), relay(src), extid(eid) {}
	~PRelay (void)	// Free ids for relays created by this Extern
	    { if (pRelay && pRelay->creator_id() == relay.src()) relay.free_id(); }
	PRelay (const PRelay&) = delete;
	void operator= (const PRelay&) = delete;
    };
    //}}}2--------------------------------------------------------------
private:
    constexpr extid_t	create_extid_from_relay_id (mrid_t id) const
			    { return id + ((_einfo.side == IExtern::SocketSide::Client) ? extid_ClientBase : extid_ServerBase); }
    PRelay*		prelay_by_extid (extid_t extid);
    PRelay*		prelay_by_id (mrid_t id);
    void		requeue_pending (void);
    bool		write_outgoing (void);
    void		read_incoming (void);
    inline bool		accept_incoming_message (void);
    inline bool		attach_to_socket (fd_t fd);
    void		enable_credentials_passing (bool enable);
private:
    fd_t		_sockfd;
    ITimer		_timer;
    streamsize		_bwritten;
    vector<ExtMsg>	_outq;		// messages queued for export
    vector<PRelay>	_relays;
    AppL::msgq_t	_pending;	// messages that created this connection
    Info		_einfo;
    streamsize		_bread;
    ExtMsg		_inmsg;		// currently incoming message
    fd_t		_infd;
};

} // namespace cwiclo
//}}}-------------------------------------------------------------------
