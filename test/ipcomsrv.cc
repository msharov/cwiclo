// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "ping.h"
#include "../xtern.h"

//----------------------------------------------------------------------
// ipcomsrv illustrates exporting the Ping interface through a socket to
// another process. The client side is implemented in ipcom.

class TestApp : public App {
public:
    static auto&	instance (void) { static TestApp s_app; return s_app; }
    void		process_args (argc_t argc, argv_t argv);
private:
			TestApp (void) : App() {}
};

CWICLO_APP (TestApp, (PingMsger),,(PPing))

//----------------------------------------------------------------------

void TestApp::process_args (argc_t argc, argv_t argv)
{
    for (int opt; 0 < (opt = getopt (argc, argv, "d"));) {
	if (opt == 'd')
	    set_flag (f_DebugMsgTrace);
	else {
	    puts ("Usage: ipcomsrv"
		#ifndef NDEBUG
		    "\n  -d\tenable debug tracing"
		#endif
		);
	    exit (EXIT_SUCCESS);
	}
    }
}
