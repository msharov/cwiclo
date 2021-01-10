// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "msg.h"
#include "sysutil.h"

//{{{ COM --------------------------------------------------------------

struct sockaddr;

namespace cwiclo {

class PCOM : public Proxy {
    DECLARE_INTERFACE (Proxy, COM, (error,"s")(export,"s")(delete,""))
public:
    explicit		PCOM (mrid_t src)		: Proxy (src) {}
    constexpr		PCOM (mrid_t src, mrid_t dest)	: Proxy (src, dest) {}
    constexpr		PCOM (Msg::Link l)		: Proxy (l) {}
			~PCOM (void)			{ }
    void		error (const string& errmsg) const	{ send (m_error(), errmsg); }
    void		export_ (const string& elist) const	{ send (m_export(), elist); }
    void		delete_ (void) const			{ send (m_delete()); }
			using Proxy::forward_msg;
    decltype(auto)	forward_msg (methodid_t imethod, Msg::Body&& body, Msg::fdoffset_t fdo, extid_t extid) const
			    { return Proxy::create_msg (imethod, move(body), fdo, extid); }
    static string	string_from_interface_list (const iid_t* elist) {
			    string elstr;
			    if (elist && *elist) {
				for (auto e = elist; *e; ++e)
				    elstr.appendf ("%s,", *e);
				elstr.pop_back();
			    }
			    return elstr;
			}
    static Msg		error_msg (const string& errmsg) {
			    Msg msg (Msg::Link{}, PCOM::m_error(), stream_sizeof(errmsg), Msg::NoFdIncluded);
			    msg.write() << errmsg;
			    return msg;
			}
    static Msg		export_msg (const string& elstr) {
			    Msg msg (Msg::Link{}, PCOM::m_export(), stream_sizeof(elstr), Msg::NoFdIncluded);
			    msg.write() << elstr;
			    return msg;
			}
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
    using Reply = PCOM;
};

//}}}-------------------------------------------------------------------
//{{{ PExtern

class PExtern : public Proxy {
    DECLARE_INTERFACE (Proxy, Extern, (open,"xib")(close,"")(connected,"q"))
public:
    //{{{2 Info
    enum class SocketSide : bool { Client, Server };
    struct Info {
	using Credentials = SocketCredentials;
    public:
	vector<iid_t>	imported;
	const iid_t*	exported;
	Credentials	creds;
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
public:
    class Reply : public Proxy::Reply {
    public:
	constexpr Reply (Msg::Link l) : Proxy::Reply(l) {}
	void connected (mrid_t extern_id) const { send (m_connected(), extern_id); }
	template <typename O>
	static constexpr bool dispatch (O* o, const Msg& msg);
	    // body below Extern because it calls Extern::lookup_by_id
    };
};

} // namespace cwiclo
//}}}-------------------------------------------------------------------
