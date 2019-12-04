// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

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
    // sequence (method1,"s")(method2,"u")(method3,"x"). Note the absence
    // of commas between parenthesis groups.
    //
    DECLARE_INTERFACE (Ping, (ping,"u"));
public:
    // Proxies are constructed with the calling object's oid.
    // Reply messages sent through reply interfaces, here PingR,
    // will be delivered to the given object.
    constexpr		PPing (mrid_t caller) : Proxy (caller) {}

    // Methods are implemented by simple marshalling of the arguments.
    // m_ping() is defined by DECLARE_INTERFACE above and returns the methodid_t of the Ping call.
    void		ping (uint32_t v) {
			    auto& msg = create_msg (m_ping(), stream_size_of(v));
			    // Here an expanded example is given with direct
			    // stream access. See PingR for a simpler example
			    // using the Send template.
			    auto os = msg.write();
			    os << v;
			    commit_msg (msg, os);
			}

    // The dispatch method is called from the destination object's
    // aggregate dispatch method. A templated implementation like
    // this can be inlined for minimum overhead. The return value
    // indicates whether the message was accepted.
    //
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() == m_ping()) {
	    // Each method unmarshals the arguments and calls the handling object
	    auto is = msg.read();
	    auto v = is.read<uint32_t>();
	    o->Ping_ping (v);	// name the handlers Interface_method by convention
	} else
	    return false;	// protocol errors are handled by caller
	return true;
    }
};

// Interfaces are always one-way. If replies are desired, a separate
// reply interface must be implemented. This is the reply interface.
//
class PPingR : public ProxyR {
    DECLARE_INTERFACE (PingR, (ping,"u"));
public:
    // Reply proxies are constructed from the owning object's creating
    // link, copied from the message that created it. ProxyR will reverse
    // the link, sending replies to the originating object.
    constexpr		PPingR (const Msg::Link& l) : ProxyR (l) {}
			// Using variadic Send is the easiest way to
			// create a message that only marshals arguments.
    void		ping (uint32_t v) { send (m_ping(), v); }
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() != m_ping())
	    return false;
	o->PingR_ping (msg.read().read<uint32_t>());
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
			    : Msger(l),_reply(l),_npings(0)
			    { LOG ("Created Ping%hu\n", msger_id()); }
			~PingMsger (void) override
			    { LOG ("Destroy Ping%hu\n", msger_id()); }
    inline void		Ping_ping (uint32_t v) {
			    LOG ("Ping%hu: %u, %u total\n", msger_id(), v, ++_npings);
			    _reply.ping (v);
			}
    bool		dispatch (Msg& msg) override {
			    return PPing::dispatch (this, msg)
					|| Msger::dispatch (msg);
			}
private:
    PPingR		_reply;
    uint32_t		_npings;
};
