// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

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
    mrid_App,	// The App is always the first Msger
    mrid_Last = numeric_limits<mrid_t>::max()-2,
    mrid_New,	// Create new Msger
    mrid_Broadcast
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
inline static constexpr auto MethodInterfaceOffset (methodid_t mid)
    { return uint8_t(mid[-1]); }
inline static constexpr auto MethodNextOffset (methodid_t mid)
    { return uint8_t(mid[-2]); }

// Interface name and methods are packed together for easy lookup
inline static constexpr iid_t InterfaceOfMethod (methodid_t __restrict__ mid)
    { return mid-MethodInterfaceOffset(mid); }
inline static constexpr auto InterfaceNameSize (iid_t iid)
    { return uint8_t(iid[-1]); }

// Signatures immediately follow the method in the pack
inline static const char* SignatureOfMethod (methodid_t __restrict__ mid)
    { return strnext(mid); }

// When unmarshalling a message, convert method name to local pointer in the interface
methodid_t LookupInterfaceMethod (iid_t iid, const char* __restrict__ mname, size_t mnamesz) noexcept;

class Msger;

// Helpers for efficiently unmarshalling lstrings
inline auto lstring_from_const_stream (const istream& is)
{
    auto ssz = *is.ptr<uint32_t>();
    auto scp = is.ptr<char>()+sizeof(ssz)-!ssz;
    if (ssz)
	--ssz;
    return lstring (scp, ssz);
}

inline auto lstring_from_stream (istream& is)
{
    auto ssz = is.readv<uint32_t>();
    auto scp = is.ptr<char>()-!ssz;
    is.skip (Align (ssz,sizeof(ssz)));
    if (ssz)
	--ssz;
    return lstring (scp, ssz);
}

//}}}-------------------------------------------------------------------
//{{{ Interface definition macros

// Use these to define the i_Interface variable in a proxy
// Example: DECLARE_INTERFACE (MyInterface, (Call1,"uix")(Call2,"x"))
// Note that the method list is not comma-separated; it is a preprocessor
// sequence with each element delimited by parentheses (a)(b)(c).

#define DECLARE_INTERFACE_METHOD_VARS(iface,mname,sig)	\
	uint8_t	method_##mname##_Size;			\
	uint8_t	method_##mname##_Offset;		\
	char	method_##mname [sizeof(#mname)];	\
	char	method_##mname##_Signature [sizeof(sig)];

#define DEFINE_INTERFACE_METHOD_VALUES(iface,mname,sig)	\
    sizeof(I##iface::method_##mname##_Size)+		\
	sizeof(I##iface::method_##mname##_Offset)+	\
	sizeof(I##iface::method_##mname)+		\
	sizeof(I##iface::method_##mname##_Signature),	\
    offsetof(I##iface, method_##mname)-offsetof(I##iface, name),\
    #mname, sig,

#define DECLARE_INTERFACE_METHOD_ACCESSORS(iface,mname,sig)\
    static constexpr methodid_t M_##mname (void) { return i_##iface.method_##mname; }

// This creates an interface definition variable as a static string
// block containing the name followed by method\0signature pairs.
// method names are preceded by size and offset bytes to allow
// obtaining interface name directly from the method name and to
// speed up lookup of method by name.
//
#define DECLARE_INTERFACE(iface,methods)\
    struct I##iface {			\
	uint8_t	name_Size;		\
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
    static constexpr iid_t Interface (void) { return i_##iface.name; }

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
    class Body : public memblock {
    public:
	using memblock::memblock;
	Body (Body&& v) : memblock(move(v)) {}
	Body (memblock&& v) : memblock(move(v)) {}
	~Body (void) noexcept;
    };
    using fdoffset_t = uint8_t;
    static constexpr fdoffset_t NoFdIncluded = numeric_limits<fdoffset_t>::max();
    struct Alignment {
	static constexpr streamsize Header = 8;
	static constexpr streamsize Body = Header;
	static constexpr streamsize Fd = alignof(int);
    };
public:
			Msg (const Link& l, methodid_t mid, streamsize size, mrid_t extid = 0, fdoffset_t fdo = NoFdIncluded) noexcept;
			Msg (const Link& l, methodid_t mid, memblock&& body, mrid_t extid = 0, fdoffset_t fdo = NoFdIncluded) noexcept;
    inline auto&	GetLink (void) const	{ return _link; }
    inline auto		Src (void) const	{ return GetLink().src; }
    inline auto		Dest (void) const	{ return GetLink().dest; }
    inline auto		Size (void) const	{ return _body.size(); }
    inline auto		Method (void) const	{ return _method; }
    inline auto		Interface (void) const	{ return InterfaceOfMethod (Method()); }
    inline auto		Signature (void) const	{ return SignatureOfMethod (Method()); }
    inline auto		Extid (void) const	{ return _extid; }
    inline void		SetExtid (mrid_t eid)	{ _extid = eid; }
    inline auto		FdOffset (void) const	{ return _fdoffset; }
    inline auto&&	MoveBody (void)		{ return move(_body); }
    inline istream	Read (void) const	{ return istream (_body); }
    inline ostream	Write (void)		{ return ostream (_body); }
    static streamsize	ValidateSignature (istream& is, const char* sig) noexcept;
    streamsize		Verify (void) const noexcept	{ auto is = Read(); return ValidateSignature (is, Signature()); }
			Msg (Msg&& msg) : Msg(msg.GetLink(),msg.Method(),msg.MoveBody(),msg.Extid(),msg.FdOffset()) {}
			Msg (Msg&& msg, const Link& l) : Msg(l,msg.Method(),msg.MoveBody(),msg.Extid(),msg.FdOffset()) {}
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
    using pfn_factory_t = Msger* (*)(const Msg::Link& l);
public:
    constexpr auto&	Link (void) const			{ return _link; }
    constexpr auto	Src (void) const			{ return Link().src; }
    constexpr auto	Dest (void) const			{ return Link().dest; }
protected:
    constexpr		ProxyB (mrid_t from, mrid_t to)		: _link {from,to} {}
			ProxyB (const ProxyB&) = delete;
    inline auto&	LinkW (void) noexcept;
    void		operator= (const ProxyB&) = delete;
    Msg&		CreateMsg (methodid_t imethod, streamsize sz) noexcept;
    void		Forward (Msg&& msg) noexcept;
#ifdef NDEBUG	// CommitMsg only does debug checking
    void		CommitMsg (Msg&, ostream&) noexcept	{ }
#else
    void		CommitMsg (Msg& msg, ostream& os) noexcept;
#endif
    inline void		Send (methodid_t imethod)		{ CreateMsg (imethod, 0); }
    template <typename... Args>
    inline void Send (methodid_t imethod, const Args&... args) {
	auto& msg = CreateMsg (imethod, variadic_stream_size(args...));
	auto os = msg.Write();
	(os << ... << args);
	CommitMsg (msg, os);
    }
private:
    Msg::Link		_link;
};

class Proxy : public ProxyB {
public:
    constexpr explicit	Proxy (mrid_t from, mrid_t to=mrid_New)	: ProxyB (from,to) {}
    void		CreateDestAs (iid_t iid) noexcept;
    void		CreateDestWith (iid_t iid, pfn_factory_t fac) noexcept;
    void		FreeId (void) noexcept;
};
class ProxyR : public ProxyB {
public:
    constexpr explicit	ProxyR (const Msg::Link& l) : ProxyB (l.dest, l.src) {}
};

//}}}-------------------------------------------------------------------
//{{{ has_msger_named_create

template <typename T> class __has_msger_named_create {
    template <typename O, T& (*)(const Msg::Link&)> struct test_for_Create {};
    template <typename O> static true_type found (test_for_Create<O,&O::Create>*);
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
    static Msger* Factory (const Msg::Link& l) {
	if constexpr (has_msger_named_create<M>::value)
	    return M::Create(l);	// this variant is used for singleton Msgers
	else
	    return new M(l);
    }
    using pfn_factory_t = ProxyB::pfn_factory_t;
    //}}}2--------------------------------------------------------------
public:
    virtual		~Msger (void) noexcept		{ }
    auto&		CreatorLink (void) const	{ return _link; }
    auto		CreatorId (void) const		{ return CreatorLink().src; }
    auto		MsgerId (void) const		{ return CreatorLink().dest; }
    auto		Flag (unsigned f) const		{ return GetBit(_flags,f); }
    static void		Error (const char* fmt, ...) noexcept PRINTFARGS(1,2);
    static void		ErrorLibc (const char* f) noexcept;
    virtual bool	Dispatch (Msg&) noexcept	{ return false; }
    virtual bool	OnError (mrid_t, const string&) noexcept
			    { SetFlag (f_Unused); return false; }
    virtual void	OnMsgerDestroyed (mrid_t mid) noexcept
			    { if (mid == CreatorId()) SetFlag (f_Unused); }
protected:
    explicit		Msger (const Msg::Link& l)	:_link(l),_flags() {}
    explicit		Msger (mrid_t id)		:_link{id,id},_flags(BitMask(f_Static)) {}
			Msger (const Msger&) = delete;
    void		operator= (const Msger&) = delete;
    void		SetFlag (unsigned f, bool v = true)	{ SetBit(_flags,f,v); }
private:
    Msg::Link		_link;
    uint32_t		_flags;
};

} // namespace cwiclo
//}}}-------------------------------------------------------------------
