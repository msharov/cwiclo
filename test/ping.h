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
// The interface object is used to send messages to the remote object, and
// contains methods mirroring those on the remote object, implemented
// by marshalling the arguments into a message. The remote object is
// created by the framework, using the a factory created by registration.
//
class IPing : public Interface {
    //
    // The interface is a char block built by the DECLARE_INTERFACE macro.
    // This creates the name of the interface followed by methods and
    // signatures in a continuous string. Here there is only one method;
    // when you have more than one, they are specified as a preprocessor
    // sequence (method1,"s")(method2,"u")(method3,"x"). Note the absence
    // of commas between parenthesis groups.
    //
    // The first argument to DECLARE_INTERFACE is the base class.
    // This is used to define base_class_t member type and recursive
    // interface collection methods used to create the interface-Msger map.
    //
    // The fourth argument specifies the socket name on which this interface
    // will be exported. The server process will listen on this socket,
    // exporting Ping interface to connections, and the imports should
    // connect to this socket to find this interface. This is roughly
    // equivalent to a DBUS well-known bus name.
    //
    // The last argument specifies the program to launch when the socket
    // is not available.
    //
    // For non-exported interfaces, DECLARE_INTERFACE macro could
    // be used instead, omitting the socket and program arguments.
    //
    DECLARE_INTERFACE_E (Interface, Ping, (ping,"u"), "@~cwiclo/test/ping.socket", "ipcomsrv");
public:
    // Interfaces are constructed with the calling object's oid,
    // to let the remote object know where to send the replies.
    // The remote object is created when the first message is
    // sent through this interface.
    //
    explicit IPing (mrid_t caller) : Interface (caller) {}

    // Methods are implemented by marshalling the arguments into a message.
    // m_ping() is defined by DECLARE_INTERFACE above and returns the
    // methodid_t of the Ping call.
    //
    void ping (uint32_t v) const { send (m_ping(), v); }

    // The dispatch method is called from the destination object's
    // aggregate dispatch method. A templated implementation like
    // this can be inlined for minimum overhead. The return value
    // indicates whether the message was accepted.
    //
    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	// Call the base interface if message method is unknown
	if (msg.method() != m_ping())
	    return Interface::dispatch (o, msg);
	//
	// Each method unmarshals the arguments and calls the handling object
	// Name the handlers Interface_method by convention
	//
	o->Ping_ping (msg.read().read<uint32_t>());
	return true;
    }
public:
    // Interfaces are always one-way. If replies are desired, a separate
    // interface must be implemented. This is Ping's reply interface. Because
    // replies must necessarily be created in response to some request, reply
    // interface methods are part of the request interface, and the reply
    // interface is always a member Reply class of the request interface.
    //
    class Reply : public Interface::Reply {
    public:
	// Reply interfaces are constructed from the owning object's creating
	// link, copied from the message that created it. Messages from this
	// interface are replies to the originating object.
	//
	constexpr Reply (Msg::Link l) : Interface::Reply (l) {}
	//
	// Here an expanded marshalling example is given with direct
	// stream access. Most of the time you should just use send.
	//
	void ping (uint32_t v) const {
	    auto& msg = create_msg (m_ping(), stream_sizeof(v));
	    auto os = msg.write();
	    os << v;
	    commit_msg (msg, os);
	}

	template <typename O>
	inline static constexpr bool dispatch (O* o, const Msg& msg) {
	    if (msg.method() == m_ping()) {
		// An example with explicitly read variables
		auto is = msg.read();
		auto v = is.read<uint32_t>();
		o->Ping_ping (v);
	    } else
		return Interface::Reply::dispatch (o, msg);
	    return true;
	}
    };
};

// An auto-flush printf for the automated tests
template <typename... Args>
static inline void log (const Args&... args)
{
    printf (args...);
    fflush (stdout);
}

//----------------------------------------------------------------------
// Finally, a server object must be defined that will implement the
// Ping interface by replying with the received argument. Each such
// object needs an IMPLEMENT_INTERFACES macro call, which will implement
// the necessary API binding interfaces to this class and dispatching
// the incoming messages to interface acceptor methods; here to Ping_ping.
// The app will use this API to map interface to Msger factory to create
// the destination object for messages sent to it.
//
// The arguments are first the base class, here Msger. Second the list
// of invokable interfaces, as their interface names. Remember that these
// are preprocessor sequences (IPing)(ISome)(IOther), with no commas.
// The third argument is another preprocessor sequence specifying
// interfaces that this Msger uses internally. These will usually be
// reply interfaces, like (IPing::Reply).
//
// Alternatively there is the IMPLEMENT_INTERFACES_I/_D pair in case
// you want to force interface methods inline, while defining their
// bodies in the .cc file. Placing IMPLEMENT_INTERFACES_D(PingMsger)
// there will generate the appropriate virtual PingMsger::dispatch.
//
class PingMsger : public Msger {
    IMPLEMENT_INTERFACES (Msger, (IPing),)
public:
    explicit		PingMsger (Msg::Link l)
			    : Msger(l),_npings(0)
			    { log ("Created Ping%hu\n", msger_id()); }
			~PingMsger (void) override
			    { log ("Destroy Ping%hu\n", msger_id()); }
    inline void		Ping_ping (uint32_t v) {
			    log ("Ping%hu: %u, %u total\n", msger_id(), v, ++_npings);
			    //
			    // Reply interface objects should generally be created
			    // when needed because the information in it
			    // is already stored in Msger as creator_link.
			    // This also allows correctly handling the case
			    // of the creator being destroyed, when no further
			    // messages should be sent to it.
			    //
			    reply<IPing>().ping (v);
			}
private:
    uint32_t		_npings;
};
