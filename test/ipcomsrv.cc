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
			TestApp (void) : App() {}
public:
    static auto&	instance (void) { static TestApp s_app; return s_app; }
};

CWICLO_APP (TestApp, (PingMsger),,(PPing))
