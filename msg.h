// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "vector.h"
#include "string.h"
#include "metamac.h"

//{{{ Types ------------------------------------------------------------
namespace cwiclo {

// Msger unique id. This is an index into the array of Msger pointers
// in the App and thus directly routes a message to its destination.
//
using mrid_t = uint16_t;
enum : mrid_t {
    // The App is always the first Msger
    mrid_App,
    // mrid space is cut in half to permit direct mapping to extid on both
    // sides of the connection. Set the exact point to 2^15-768 = 32000,
    // 0x7d00, mostly round number in both bases; helpful in the debugger)
    mrid_Last = numeric_limits<mrid_t>::max()/2-768-1,
    // placeholder requesting new msger creation
    mrid_New = numeric_limits<mrid_t>::max()-1,
    mrid_Broadcast	// indicates all msgers as destination
};

// Interfaces are defined as string blocks with the interface
// name serving as a unique id for the interface.
//
using iid_t = const char*;

// Interface methods are likewise uniquely identified by their
// name string. This allows creating a message with a methodid_t as
// the only argument. Message destination method is also easily
// visible in the debugger.
//
using methodid_t = const char*;

// Methods are preceded by indexes to interface and next method
static constexpr auto method_interface_offset (methodid_t mid)
    { return uint8_t(mid[-1]); }
static constexpr auto method_next_offset (methodid_t mid)
    { return uint8_t(mid[-2]); }

// Interface name and methods are packed together for easy lookup
static constexpr iid_t interface_of_method (methodid_t __restrict__ mid)
    { return mid-method_interface_offset(mid); }
static constexpr auto interface_name_size (iid_t iid)
    { return uint8_t(iid[-1]); }

// Signatures immediately follow the method in the pack
static constexpr const char* signature_of_method (methodid_t __restrict__ mid)
    { return mid+__builtin_strlen(mid)+1; }

// When unmarshalling a message, convert method name to local pointer in the interface
methodid_t interface_lookup_method (iid_t iid, const char* __restrict__ mname, size_t mnamesz) noexcept;

class Msger;

// Helpers for efficiently unmarshalling string_views
constexpr auto string_view_from_const_stream (const istream& is)
{
    auto ssz = *is.ptr<uint32_t>();
    auto scp = is.ptr<char>()+sizeof(ssz)-!ssz;
    if (ssz)
	--ssz;
    return string_view (scp, ssz);
}

constexpr auto string_view_from_stream (istream& is)
{
    auto ssz = is.read<uint32_t>();
    auto scp = is.ptr<char>()-!ssz;
    is.skip (ceilg (ssz,sizeof(ssz)));
    if (ssz)
	--ssz;
    return string_view (scp, ssz);
}

//}}}-------------------------------------------------------------------
//{{{ Interface definition macros

// Use these to define the i_Interface variable in a proxy
// Example: DECLARE_INTERFACE (MyInterface, (Call1,"uix")(Call2,"x"))
// Note that the method list is not comma-separated; it is a preprocessor
// sequence with each element delimited by parentheses (a)(b)(c).

#define DECLARE_INTERFACE_METHOD_VARS(iface,mname,sig)	\
	uint8_t	method_##mname##_size;			\
	uint8_t	method_##mname##_offset;		\
	char	method_##mname [sizeof(#mname)];	\
	char	method_##mname##_signature [sizeof(sig)];

#define DEFINE_INTERFACE_METHOD_VALUES(iface,mname,sig)	\
    sizeof(I##iface::method_##mname##_size)+		\
	sizeof(I##iface::method_##mname##_offset)+	\
	sizeof(I##iface::method_##mname)+		\
	sizeof(I##iface::method_##mname##_signature),	\
    offsetof(I##iface, method_##mname)-offsetof(I##iface, name),\
    #mname, sig,

#define DECLARE_INTERFACE_METHOD_ACCESSORS(iface,mname,sig)\
    static constexpr methodid_t m_##mname (void) { return i_##iface.method_##mname; }

// This creates an interface definition variable as a static string
// block containing the name followed by method\0signature pairs.
// method names are preceded by size and offset bytes to allow
// obtaining interface name directly from the method name and to
// speed up lookup of method by name.
//
#define DECLARE_INTERFACE(iface,methods)\
    struct I##iface {			\
	uint8_t	name_size;		\
	char	name [sizeof(#iface)];	\
	SEQ_FOR_EACH (methods, iface, DECLARE_INTERFACE_METHOD_VARS)\
	uint8_t	endsign;		\
    };					\
    static constexpr const I##iface i_##iface = {\
	sizeof(#iface), #iface,		\
	SEQ_FOR_EACH (methods, iface, DEFINE_INTERFACE_METHOD_VALUES)\
	0				\
    };					\
    SEQ_FOR_EACH (methods, iface, DECLARE_INTERFACE_METHOD_ACCESSORS)\
public:					\
    static constexpr iid_t interface (void) { return i_##iface.name; }

// This one instantiates the i_Interface variable from above
#define DEFINE_INTERFACE(iface)		\
    constexpr P##iface::I##iface P##iface::i_##iface;

//}}}-------------------------------------------------------------------
//{{{ Msg

class Msg {
public:
    struct Link {
	mrid_t	src;
	mrid_t	dest;
    };
    using Body		= memblaz;
    using value_type	= Body::value_type;
    using iterator	= Body::iterator;
    using const_iterator = Body::const_iterator;
    using size_type	= streamsize;
    using fd_t		= int32_t;
    using fdoffset_t	= uint8_t;
    static constexpr fdoffset_t NoFdIncluded = numeric_limits<fdoffset_t>::max();
    struct Alignment {
	static constexpr streamsize Header = 8;
	static constexpr streamsize Body = Header;
	static constexpr streamsize Fd = alignof(fd_t);
    };
    enum { MaxSize = (1u<<24)-1 };
public:
			Msg (const Link& l, methodid_t mid, streamsize size, fdoffset_t fdo = NoFdIncluded) noexcept;
    constexpr auto&	link (void) const	{ return _link; }
    constexpr auto	src (void) const	{ return _link.src; }
    constexpr auto	dest (void) const	{ return _link.dest; }
    constexpr auto	size (void) const	{ return _body.size(); }
    constexpr auto	max_size (void) const	{ return MaxSize; }
    constexpr auto	data (void) const	{ return _body.data(); }
    constexpr auto	data (void)		{ return _body.data(); }
    constexpr auto	begin (void) const	{ return _body.begin(); }
    constexpr auto	begin (void)		{ return _body.begin(); }
    constexpr auto	end (void) const	{ return _body.end(); }
    constexpr auto	end (void)		{ return _body.end(); }
    constexpr auto	method (void) const	{ return _method; }
    constexpr auto	interface (void) const	{ return interface_of_method (method()); }
    constexpr auto	signature (void) const	{ return signature_of_method (method()); }
    constexpr auto	extid (void) const	{ return _extid; }
    constexpr auto	fd_offset (void) const	{ return _fdoffset; }
    constexpr auto&&	move_body (void)	{ return move(_body); }
    inline constexpr auto read (void) const	{ return istream (data(),size()); }
    inline constexpr auto write (void)		{ return ostream (data(),size()); }
    static streamsize	validate_signature (istream is, const char* sig) noexcept;
    auto		verify (void) const noexcept	{ return validate_signature (read(), signature()); }
    inline constexpr	Msg (const Link& l, methodid_t mid) noexcept
			    :_method (mid),_link (l),_extid(0),_fdoffset (NoFdIncluded),_body() {}
    inline constexpr	Msg (const Link& l, methodid_t mid, Body&& body, fdoffset_t fdo = NoFdIncluded, mrid_t extid = 0) noexcept
			    :_method (mid),_link (l),_extid (extid),_fdoffset (fdo),_body (move (body)) {}
    inline constexpr	Msg (Msg&& msg, const Link& l) noexcept
			    : Msg (l,msg.method(),msg.move_body(),msg.fd_offset(),msg.extid()) {}
    inline constexpr	Msg (Msg&& msg) noexcept
			    : Msg (move(msg), msg.link()) {}
			Msg (const Msg&) = delete;
    Msg&		operator= (const Msg&) = delete;
private:
    methodid_t		_method;
    Link		_link;
    mrid_t		_extid;
    fdoffset_t		_fdoffset;
    Body		_body;
};

//}}}-------------------------------------------------------------------
//{{{ Proxy

class ProxyB {
public:
    using fd_t = Msg::fd_t;
    using pfn_factory_t = Msger* (*)(const Msg::Link& l);
public:
    constexpr auto&	link (void) const			{ return _link; }
    constexpr auto	src (void) const			{ return link().src; }
    constexpr auto	dest (void) const			{ return link().dest; }
protected:
    constexpr		ProxyB (mrid_t from, mrid_t to)		: _link {from,to} {}
			ProxyB (const ProxyB&) = delete;
    inline auto&	linkw (void) noexcept;
    void		operator= (const ProxyB&) = delete;
    Msg&		create_msg (methodid_t imethod, streamsize sz) noexcept;
    Msg&		create_msg (methodid_t imethod, streamsize sz, Msg::fdoffset_t fdo) noexcept;
    void		forward_msg (Msg&& msg) noexcept;
#ifdef NDEBUG	// CommitMsg only does debug checking
    void		commit_msg (Msg&, ostream&) noexcept	{ }
#else
    void		commit_msg (Msg& msg, ostream& os) noexcept;
#endif
    inline void		send (methodid_t imethod)		{ create_msg (imethod, 0); }
    template <typename... Args>
    inline void		send (methodid_t imethod, const Args&... args) {
			    auto& msg = create_msg (imethod, variadic_stream_size(args...));
			    commit_msg (msg, (msg.write() << ... << args));
			}
    inline void		send_fd (methodid_t imethod, fd_t fd) {
			    assert (string_view("h") == signature_of_method(imethod));
			    auto& msg = create_msg (imethod, stream_size_of(fd), 0);
			    commit_msg (msg, msg.write() << fd);
			}
private:
    Msg::Link		_link;
};

class Proxy : public ProxyB {
public:
    constexpr		Proxy (mrid_t from, mrid_t to=mrid_New)	: ProxyB (from,to) {}
    void		create_dest_as (iid_t iid) noexcept;
    void		create_dest_with (iid_t iid, pfn_factory_t fac) noexcept;
    void		free_id (void) noexcept;
};
class ProxyR : public ProxyB {
public:
    constexpr		ProxyR (const Msg::Link& l) : ProxyB (l.dest, l.src) {}
};

//}}}-------------------------------------------------------------------
//{{{ has_msger_named_create

template <typename T> class __has_msger_named_create {
    template <typename O, T& (*)(const Msg::Link&)> struct test_for_create {};
    template <typename O> static true_type found (test_for_create<O,&O::create>*);
    template <typename O> static false_type found (...);
public:
    using type = decltype(found<T>(nullptr));
};
// Differentiates between normal and singleton Msger classes
template <typename T>
struct has_msger_named_create : public __has_msger_named_create<T>::type {};

//}}}-------------------------------------------------------------------
//{{{ Msger

class Msger {
public:
    enum { f_Unused, f_Static, f_Last };
    //{{{2 Msger factory template --------------------------------------
    template <typename M>
    [[nodiscard]] static Msger* factory (const Msg::Link& l) {
	if constexpr (has_msger_named_create<M>::value)
	    return M::create(l);	// this variant is used for singleton Msgers
	else
	    return new M(l);
    }
    using pfn_factory_t = ProxyB::pfn_factory_t;
    //}}}2--------------------------------------------------------------
public:
    virtual		~Msger (void) noexcept		{ }
    constexpr auto&	creator_link (void) const	{ return _link; }
    constexpr auto	creator_id (void) const		{ return creator_link().src; }
    constexpr auto	msger_id (void) const		{ return creator_link().dest; }
    constexpr auto	flag (unsigned f) const		{ return get_bit (_flags,f); }
    static void		error (const char* fmt, ...) noexcept PRINTFARGS(1,2);
    static void		error_libc (const char* f) noexcept;
    virtual bool	dispatch (Msg&) noexcept	{ return false; }
    virtual bool	on_error (mrid_t, const string&) noexcept
			    { set_unused(); return false; }
    virtual void	on_msger_destroyed (mrid_t mid) noexcept
			    { if (mid == creator_id()) set_unused(); }
protected:
    explicit constexpr	Msger (const Msg::Link& l)	:_link(l),_flags() {}
    explicit constexpr	Msger (mrid_t id)		:_link{id,id},_flags(bit_mask(f_Static)) {}
			Msger (const Msger&) = delete;
    void		operator= (const Msger&) = delete;
    constexpr void	set_flag (unsigned f, bool v = true)	{ set_bit (_flags,f,v); }
    constexpr void	set_unused (bool v = true)	{ set_flag (f_Unused, v); }
private:
    Msg::Link		_link;
    uint32_t		_flags;
};

} // namespace cwiclo
//}}}-------------------------------------------------------------------
