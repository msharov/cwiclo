// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "ping.h"

class TestApp : public App {
public:
    // Apps always use the singleton pattern
    static auto& instance (void) { static TestApp s_app; return s_app; }

    bool dispatch (Msg& msg) override {
	//
	// Every Msger must implement the dispatch virtual,
	// listing all interfaces it responds to. Here, the
	// TestApp receives PingR messages. PPingR::dispatch
	// will accept messages for PingR interface and return
	// true when they are handled.
	//
	return PPingR::dispatch (this, msg)
	    //
	    // The base class dispatch will handle unrecognized
	    // messages by returning false.
	    //
	    || App::dispatch (msg);
    }
    void PingR_ping (uint32_t v) {
	//
	// This method is called by PPingR::dispatch when a
	// Ping message is received on a PingR interface.
	// Note the naming convention.
	//
	LOG ("Ping %u reply received in app\n", v);
	if (++v < 5)
	    _pinger.ping (v);
	else
	    quit();
    }
private:
    TestApp (void)
    : App()
    //
    // Remote Msgers are accessed through an interface proxy,
    // here of type PPing. The proxy will have methods that
    // are called as if it were a real object, but will instead
    // marshal the arguments and send them to the remote
    // interface instance.
    //
    // Proxies require the source mrid, to tell the destination
    // Msger where the message came from and where to reply.
    // Usually the mrid is obtained by calling msger_id(), but
    // the App object is always mrid_App, so it can be used directly.
    //
    ,_pinger (mrid_App)
    {
	// When ready, simply call the desired method.
	// All method calls are asynchonous, never blocking.
	// Replies arrive to PingR_ping above when sent
	// by the Ping server in this same manner.
	//
	_pinger.ping (1);
    }
private:
    PPing		_pinger;
};

// main is easiest to create by using these macros
BEGIN_CWICLO_APP (TestApp)
    //
    // Each interface must be associated with a Msger object that is to be
    // created as its implementation. Here, PingMsger responds to Ping interface.
    //
    REGISTER_MSGER (PPing, PingMsger)
END_CWICLO_APP
