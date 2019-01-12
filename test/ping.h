// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

// You would normally include cwiclo.h this way
//#include <cwiclo.h>

// But for the purpose of building the tests, local headers are
// included directly to ensure they are used instead of installed.
#include "../app.h"
using namespace cwiclo;

//----------------------------------------------------------------------
// For communication between objects, interfaces must be defined for
// both sides of the link, as well as method marshalling and dispatch.
// The proxy "object" is used to send messages to the remote object, and
// contains methods mirroring those on the remote object, implemented
// by marshalling the arguments into a message. The remote object is
// created by the framework, using the a factory created by registration.
//
// This is the calling side for the Ping interface
class PPing : public Proxy {
    // The interface is a char block built by the DECLARE_INTERFACE macro.
    // This creates the name of the interface followed by methods and
    // signatures in a continuous string. Here there is only one method;
    // when you have more than one, they are specified as a preprocessor
    // sequence (Method1,"s")(Method2,"u")(Method3,"x"). Note the absence
    // of commas between parenthesis groups.
    //
    DECLARE_INTERFACE (Ping, (Ping,"u"));
public:
    // Proxies are constructed with the calling object's oid.
    // Reply messages sent through reply interfaces, here PingR,
    // will be delivered to the given object.
			PPing (mrid_t caller) : Proxy (caller) {}
    // Methods are implemented by simple marshalling of the arguments.
    // M_Ping() is defined by DECLARE_INTERFACE above and returns the methodid_t of the Ping call.
    void		Ping (uint32_t v) {
			    auto& msg = CreateMsg (M_Ping(), stream_size_of(v));
			    // Here an expanded example is given with direct
			    // stream access. See PingR for a simpler example
			    // using the Send template.
			    auto os = msg.Write();
			    os << v;
			    CommitMsg (msg, os);
			}

    // The Dispatch method is called from the destination object's
    // aggregate Dispatch method. A templated implementation like
    // this can be inlined for minimum overhead. The return value
    // indicates whether the message was accepted.
    //
    template <typename O>
    inline static bool Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() == M_Ping()) {
	    // Each method unmarshals the arguments and calls the handling object
	    auto is = msg.Read();
	    uint32_t v; is >> v;
	    o->Ping_Ping (v);	// name the handlers Interface_Method by convention
	} else
	    return false;	// protocol errors are handled by caller
	return true;
    }
};

// Interfaces are always one-way. If replies are desired, a separate
// reply interface must be implemented. This is the reply interface.
//
class PPingR : public ProxyR {
    DECLARE_INTERFACE (PingR, (Ping,"u"));
public:
    // Reply proxies are constructed from the owning object's creating
    // link, copied from the message that created it. ProxyR will reverse
    // the link, sending replies to the originating object.
			PPingR (const Msg::Link& l) : ProxyR (l) {}
			// Using variadic Send is the easiest way to
			// create a message that only marshals arguments.
    void		Ping (uint32_t v) { Send (M_Ping(), v); }
    template <typename O>
    inline static bool Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() != M_Ping())
	    return false;
	o->PingR_Ping (msg.Read().readv<uint32_t>());
	return true;
    }
};

#define LOG(...)	do {printf(__VA_ARGS__);fflush(stdout);} while(false)

//----------------------------------------------------------------------
// Finally, a server object must be defined that will implement the
// Ping interface by replying with the received argument.
//
class PingMsger : public Msger {
public:
    explicit		PingMsger (const Msg::Link& l)
			    : Msger(l),_reply(l),_nPings(0)
			    { LOG ("Created Ping%hu\n", MsgerId()); }
			~PingMsger (void) noexcept override
			    { LOG ("Destroy Ping%hu\n", MsgerId()); }
    inline void		Ping_Ping (uint32_t v) {
			    LOG ("Ping%hu: %u, %u total\n", MsgerId(), v, ++_nPings);
			    _reply.Ping (v);
			}
    bool		Dispatch (Msg& msg) noexcept override {
			    return PPing::Dispatch (this, msg)
					|| Msger::Dispatch (msg);
			}
private:
    PPingR		_reply;
    uint32_t		_nPings;
};
