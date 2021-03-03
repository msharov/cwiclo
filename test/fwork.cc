// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "ping.h"

class TestApp : public AppL {
    //
    // To use COM, each object must use IMPLEMENT_INTERFACES to declare
    // the interfaces it uses. The arguments are the base class, the list
    // of interfaces this Msger is created for, and the list of interfaces
    // this Msger uses and gets replies from. The lists are preprocessor
    // sequences with parentheses and no commas: (IPing)(POther)(PElse).
    // The lists contain names of interface classes.
    //
    IMPLEMENT_INTERFACES (AppL,,(IPing))
public:
    // Apps always use the singleton pattern
    static auto& instance (void) { static TestApp s_app; return s_app; }
    //
    // This method is called by IPing::dispatch when a
    // Ping message is received on a Ping interface.
    // Note the naming convention.
    //
    void Ping_ping (uint32_t v) {
	log ("Ping %u reply received in app\n", v);
	if (++v < 5)
	    _pinger.ping (v);
	else
	    quit();
    }
private:
    //
    // Remote Msgers are accessed through an interface object,
    // here of type IPing. The interface object will have methods that
    // are called as if it were a real object, but will instead marshal
    // the arguments and send them to the remote interface instance.
    //
    // Interface objects require the source mrid, to tell the destination
    // Msger where the message came from and where to reply.
    // Usually the mrid is obtained by calling msger_id(), but
    // the App object is always mrid_App, so it can be used directly.
    //
    TestApp (void) : AppL(),_pinger (mrid_App) {
	// When ready, simply call the desired method.
	// All method calls are asynchonous, never blocking.
	// Replies arrive to Ping_ping above when sent
	// by the Ping server in this same manner.
	//
	_pinger.ping (1);
    }
private:
    IPing _pinger;
};

// main should be created by using CWICLO_APP macros.
// The arguments are the app class and the list of used msgers.
// Here these are TestApp, and the PingMsger object with which
// the app will communicate using the IPing object. PingMsger
// lists the Ping interface as invokable, so this macro here
// will map the Ping interface to PingMsger factory.
//
CWICLO_APP_L (TestApp, (PingMsger))
