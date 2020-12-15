// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "ping.h"
#include "../xcom.h"

//----------------------------------------------------------------------
// ipcomsrv illustrates exporting the Ping interface through a socket to
// another process. The client side is implemented in ipcom.

class TestApp : public App {
public:
    static auto&	instance (void) { static TestApp s_app; return s_app; }
    void		process_args (argc_t argc, argv_t argv);
    bool		dispatch (Msg& msg) override
			    { return PExtern::Reply::dispatch (this, msg) || App::dispatch (msg); }
    inline void		Extern_connected (const Extern::Info*);
private:
			TestApp (void) : App(), _eserver (mrid_App) {}
private:
    PExternServer	_eserver;
};

BEGIN_CWICLO_APP (TestApp)
    REGISTER_MSGER (PPing, PingMsger)
    REGISTER_MSGER (PExternServer, ExternServer)
    REGISTER_EXTERNS
END_CWICLO_APP

//----------------------------------------------------------------------

void TestApp::process_args (argc_t argc, argv_t argv)
{
    for (int opt; 0 < (opt = getopt (argc, argv, "d"));) {
	if (opt == 'd')
	    set_flag (f_DebugMsgTrace);
	else {
	    puts ("Usage: ipcom"
		#ifndef NDEBUG
		    "\n  -d\tenable debug tracing"
		#endif
		);
	    exit (EXIT_SUCCESS);
	}
    }
    // When you run a msger server, you must specify a list of interfaces
    // it is capable of instantiating. In this example, the only interface
    // exported is Ping (see ping.h). The client side sends an empty export
    // list when it only imports interfaces.
    static const iid_t eil_Ping[] = { PPing::interface(), nullptr };

    // Object servers can be run on a UNIX socket or a TCP port. ipcom shows
    // the UNIX socket version. These sockets are created in system standard
    // locations; typically /run for root processes or /run/user/<uid> for
    // processes launched by the user. If you also implement systemd socket
    // activation (see below), any other sockets can be used.
    static const char c_IPCOM_socket_name[] = "@ipcom.socket";

    // When writing a server, it is recommended to use the ExternServer
    // object to manage the sockets being listened to. It will create
    // Extern objects for each accepted connection. For the purpose of
    // this example, only one socket is listened on, but additional
    // ExternServer objects can be created to serve on more sockets.
    //
    // The activate calls are the recommended implementation method.
    // They support systemd socket activation and will try that first.
    // If no sockets are passed in, then the named socket is created.
    // Here, activate_user_local will create ipcom.socket in
    // XDG_RUNTIME_DIR. Abstract sockets are supported with @names.
    //
    // PExternServer calls return the fd of the new socket, -1 on failure.
    //
    if (0 > _eserver.activate_user_local (c_IPCOM_socket_name, eil_Ping))
	return error_libc ("activate_user_local");
}

void TestApp::Extern_connected (const Extern::Info*)
{
    // When a client connects, ExternServer will forward the
    // Extern_connected message here. On the server side,
    // there is nothing to do in this example.
}
