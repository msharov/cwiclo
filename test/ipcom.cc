// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "ping.h"
#include "../xtern.h"

//----------------------------------------------------------------------
// ipcom illustrates communicating with Msgers in another process.
// The code is almost the same as in fwork.cc, but the TestApp object is
// now derived from extern-enabled App, and CWICLO_APP now contains the
// list of imports. Connection to the server socket, or launching of the
// ipcomsrv process is handled automatically by App.

class TestApp : public App {
    IMPLEMENT_INTERFACES (App,,(IPing))
public:
    static auto& instance (void) { static TestApp s_app; return s_app; }
    void Ping_ping (uint32_t v) {
	log ("Ping %u reply received in app\n", v);
	if (++v < 5)
	    _pinger.ping (v);
	else
	    quit();
    }
private:
    TestApp (void) : App(),_pinger (mrid_App) { _pinger.ping (1); }
private:
    IPing _pinger;
};

CWICLO_APP (TestApp,,(IPing),)
