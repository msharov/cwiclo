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
    static auto&	Instance (void) { static TestApp s_App; return s_App; }
    void		ProcessArgs (argc_t argc, argv_t argv) noexcept;
    bool		Dispatch (Msg& msg) noexcept override
			    { return PExternR::Dispatch (this, msg) || App::Dispatch (msg); }
    inline void		ExternR_Connected (const ExternInfo*) noexcept;
private:
			TestApp (void) : App(), _eserver (mrid_App), _epipe (mrid_App) {}
private:
    PExternServer	_eserver;
    PExtern		_epipe;
};

BEGIN_CWICLO_APP (TestApp)
    REGISTER_MSGER (Ping, PingMsger)
    REGISTER_MSGER (ExternServer, ExternServer)
    REGISTER_EXTERNS
END_CWICLO_APP

//----------------------------------------------------------------------

void TestApp::ProcessArgs (argc_t argc, argv_t argv) noexcept
{
    bool isPrivatePipe = false;
    for (int opt; 0 < (opt = getopt (argc, argv, "pd"));) {
	if (opt == 'p')
	    isPrivatePipe = true;
	#ifndef NDEBUG
	    else if (opt == 'd')
		SetFlag (f_DebugMsgTrace);
	#endif
	else {
	    printf ("Usage: ipcom [-p]\n"
		    #ifndef NDEBUG
			"  -d\tenable debug tracing\n"
		    #endif
		    "  -p\tattach to socket pipe on stdin\n");
	    exit (EXIT_SUCCESS);
	}
    }
    // When you run a msger server, you must specify a list of interfaces
    // it is capable of instantiating. In this example, the only interface
    // exported is Ping (see ping.h). The client side sends an empty export
    // list when it only imports interfaces.
    static const iid_t eil_Ping[] = { PPing::Interface(), nullptr };

    // Object servers can be run on a UNIX socket or a TCP port. ipcom shows
    // the UNIX socket version. These sockets are created in system standard
    // locations; typically /run for root processes or /run/user/<uid> for
    // processes launched by the user. If you also implement systemd socket
    // activation (see below), any other sockets can be used.
    static const char c_IPCOM_SocketName[] = "ipcom.socket";

    // When writing a server, it is recommended to use the ExternServer
    // object to manage the sockets being listened to. It will create
    // Extern objects for each accepted connection. For the purpose of
    // this example, only one socket is listened on, but additional
    // ExternServer objects can be created to serve on more sockets.
    //
    // ExternServer will listen on a socket. For a private pipe, accept
    // is not required, and only an Extern Msger is needed to directly
    // bind to the pipe file descriptor.
    //
    if (isPrivatePipe)
	_epipe.Open (STDIN_FILENO, eil_Ping);
    // 
    // For flexibility it is recommended to implement systemd socket activation.
    // Doing so is very simple; sd_listen_fds returns the number of fds passed to
    // the process by systemd. The fds start at SD_LISTEN_FDS_START. To keep the
    // example simple, only the first one is used; create more ExternServer objects
    // to listen on more than one socket.
    //
    else if (sd_listen_fds())
	_eserver.Open (SD_LISTEN_FDS_START+0, eil_Ping);
    //
    // If no sockets were passed from systemd, create one manually
    // Use BindUserLocal to create the socket in XDG_RUNTIME_DIR,
    // the standard location for sockets of user services.
    // See other Bind variants in PExternServer declaration.
    // Each returns the fd of the new socket, -1 on failure.
    //
    else if (0 > _eserver.BindUserLocal (c_IPCOM_SocketName, eil_Ping))
	return ErrorLibc ("BindUserLocal");
}

void TestApp::ExternR_Connected (const ExternInfo*) noexcept
{
    // When a client connects, ExternServer will forward the
    // ExternR_Connected message here. On the server side,
    // there is nothing to do in this example.
}
