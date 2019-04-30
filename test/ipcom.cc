// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "ping.h"
#include "../xcom.h"
#include <sys/wait.h>
#include <signal.h>

//----------------------------------------------------------------------
// ipcom illustrates communicating with Msgers in another process

class TestApp : public App {
public:
    static auto&	Instance (void) noexcept { static TestApp s_App; return s_App; }
    bool		Dispatch (Msg& msg) noexcept override {
			    return PPingR::Dispatch (this, msg)
				|| PExternR::Dispatch (this, msg)
				|| PSignal::Dispatch (this, msg)
				|| App::Dispatch (msg);
			}
    void		OnMsgerDestroyed (mrid_t mid) noexcept override;
    void		ProcessArgs (argc_t argc, argv_t argv) noexcept;
    inline void		ExternR_Connected (const ExternInfo* einfo) noexcept;
    inline void		PingR_Ping (uint32_t v) noexcept;
    inline void		Signal_Signal (int sig) noexcept;
private:
			TestApp (void) noexcept;
private:
    PPing		_pinger;
    PExtern		_eclient;
};

BEGIN_CWICLO_APP (TestApp)
    REGISTER_EXTERN_MSGER (Ping)
    REGISTER_EXTERN_MSGER (PingR)
    REGISTER_EXTERNS
END_CWICLO_APP

//----------------------------------------------------------------------

TestApp::TestApp (void) noexcept
: App()
,_pinger (mrid_App)
,_eclient (mrid_App)
{
    // In the previous example, the _pinger proxy was immediately used
    // to send the Ping message. Here, a connection to an external process
    // exporting the Ping interface must first be established.
}

void TestApp::ProcessArgs (argc_t argc [[maybe_unused]], argv_t argv [[maybe_unused]]) noexcept
{
    #ifndef NDEBUG
	// Debug tracing is very useful in asynchronous apps, since backtraces
	// no longer have much meaning. A list of messages exchanged is the
	// usual debugging tool, used much like a network packet sniffer.
	for (int opt; 0 < (opt = getopt (argc, argv, "d"));)
	    if (opt == 'd')
		SetFlag (f_DebugMsgTrace);
    #endif
    // Msger servers can be run on a UNIX socket or a TCP port. ipcom shows
    // the UNIX socket version. These sockets are created in system standard
    // locations; typically /run for root processes or /run/user/<uid> for
    // processes launched by the user. If you also implement systemd socket
    // activation (see below), any other sockets can be used.
    static const char c_IPCOM_SocketName[] = "ipcom.socket";

    // Communication between processes requires the use of the Extern
    // interface and auxillary factories for creating passthrough local
    // objects representing both the remote and local addressable objects.
    //
    // The client side creates the Extern object on an fd connected to
    // the server socket is given by it the list of imported interfaces.
    // Imported interface instances can then be transparently created by
    // sending them a message with a proxy, just as if they were local.
    //
    // ConnectUserLocal tries connecting to $XDG_RUNTIME_DIR/ipcom.socket
    //
    if (0 > _eclient.ConnectUserLocal (c_IPCOM_SocketName))
	//
	// The return value is the opened fd. -1 if the open failed.
	// Automated testing in the Makefile will run only ipcom, so
	// the server is launched manually here on a private pipe.
	//
	if (0 > _eclient.LaunchPipe ("ipcomsrv", "-p"))
	    return ErrorLibc ("LaunchPipe");

    // Now wait for ExternR_Connected
}

// When a client connects, Extern will forward the ExternR_Connected message
// here. This is the appropriate place to check that all the imports are satisfied,
// authenticate the connection (if using a UNIX socket), and create objects.
//
void TestApp::ExternR_Connected (const ExternInfo* einfo) noexcept
{
    // ExternInfo contains the list of interfaces available on this connection, so
    // here is the right place to check that the expected interfaces can be created.
    //
    if (!einfo->IsImporting (PPing::Interface()))
	return Error ("connected to server that does not support the Ping interface");
    LOG ("Connected to server. Imported %u interface: %s\n", einfo->imported.size(), einfo->imported[0]);

    // The ExternInfo also specifies whether the connection is a UNIX
    // socket (allowing passing fds and credentials), and client process
    // credentials, if available. This information may be useful for choosing
    // file transmission mode, or for authentication.
    //
    // Remote objects are created and used in exactly the same manner as
    // local ones, by creating a proxy and calling its methods.
    //
    _pinger.Ping (1);
}

// The ping replies are received exactly the same way as from a local
// object. But now the object resides in the server process and the
// message is read from the socket.
void TestApp::PingR_Ping (uint32_t v) noexcept
{
    LOG ("Ping %u reply received in app\n", v);
    if (++v < 5)
	_pinger.Ping (v);
    else
	Quit();
}

// This test scenario launches a private pipe server,
// and so must cleanup the child process when it exits.
// Signal messages are sent by the app, broadcasting to
// all recepients, so there is no need to register for it,
// and this method is implemented like a signal handler.
//
void TestApp::Signal_Signal (int sig) noexcept
{
    if (sig == SIGCHLD) {
	int ec = EXIT_SUCCESS;
	auto spid = waitpid (-1, &ec, WNOHANG);
	if (spid > 0 && WIFEXITED(ec)) {
	    LOG ("Child process %d exited with code %d\n", spid, ec);
	    Quit();
	}
    }
}

// In case of errors, the remote end usually forwards an informative
// message. However, if the error occurs in the remote Extern, or in
// the socket itself, the connection may get dropped without explanation.
// The remote end may also have been shut down normally, without an
// error. Here this can be detected when undesirable.
//
void TestApp::OnMsgerDestroyed (mrid_t mid) noexcept
{
    App::OnMsgerDestroyed (mid);
    if (Flag (f_Quitting))
	return;	// during normal operation, the client initiates shutdown
    if (mid == _pinger.Dest())
	LOG ("Error: remote Ping object was unexpectedly destroyed\n");
    else if (mid == _eclient.Dest())
	LOG ("Error: remote connection terminated unexpectedly\n");
    Quit();
}
