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

class ICOM : public Interface {
    DECLARE_INTERFACE (Interface, COM, (error,"s")(export,"s")(delete,""))
public:
    explicit		ICOM (mrid_t src)		: Interface (src) {}
    constexpr		ICOM (mrid_t src, mrid_t dest)	: Interface (src, dest) {}
    constexpr		ICOM (Msg::Link l)		: Interface (l) {}
			~ICOM (void)			{ }
    void		error (const string& errmsg) const	{ send (m_error(), errmsg); }
    void		export_ (const string& elist) const	{ send (m_export(), elist); }
    void		delete_ (void) const			{ send (m_delete()); }
			using Interface::forward_msg;
    decltype(auto)	forward_msg (methodid_t imethod, Msg::Body&& body, Msg::fdoffset_t fdo, extid_t extid) const
			    { return Interface::create_msg (imethod, move(body), fdo, extid); }
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
			    Msg msg (Msg::Link{}, ICOM::m_error(), stream_sizeof(errmsg), Msg::NoFdIncluded);
			    msg.write() << errmsg;
			    return msg;
			}
    static constexpr bool allowed_before_auth (methodid_t mid) { return mid == m_export(); }
    static Msg		export_msg (const string& elstr) {
			    Msg msg (Msg::Link{}, ICOM::m_export(), stream_sizeof(elstr), Msg::NoFdIncluded);
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
    using Reply = ICOM;
};

//}}}-------------------------------------------------------------------
//{{{ IExtern

class IExtern : public Interface {
    DECLARE_INTERFACE (Interface, Extern, (open,"xib")(close,""))
public:
    //{{{2 Info
    enum class SocketSide : bool { Client, Server };
    struct Info {
	using Credentials = SocketCredentials;
    public:
	vector<iid_t>	imported;
	const iid_t*	exported;
	Credentials	creds;
	uid_t		filter_uid;
	mrid_t		extern_id;
	SocketSide	side;
	bool		is_local_socket;
	bool		is_connected;
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
    explicit	IExtern (mrid_t caller)	: Interface(caller) {}
		~IExtern (void)		{ free_id(); }
    void	close (void) const	{ send (m_close()); }
    void	open (fd_t fd, const iid_t* eifaces, SocketSide side = SocketSide::Server) const
		    { send (m_open(), ios::ptr(eifaces), fd, side); }
    void	open (fd_t fd) const	{ open (fd, nullptr, SocketSide::Client); }
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

} // namespace cwiclo
//}}}-------------------------------------------------------------------
