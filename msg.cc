// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "msg.h"
#include "app.h"
#include <stdarg.h>

namespace cwiclo {

methodid_t LookupInterfaceMethod (iid_t iid, const char* __restrict__ mname, size_t mnamesz) noexcept
{
    for (methodid_t __restrict__ mid = iid+iid[-1]; mid[0]; mid += mid[0])
	if (uint8_t(mid[0]-2) == mnamesz && 0 == memcmp (mname, mid+2, mnamesz))
	    return mid+2;
    return nullptr;
}

//----------------------------------------------------------------------

auto& ProxyB::LinkW (void) noexcept { return _link; }

Msg& ProxyB::CreateMsg (methodid_t mid, streamsize sz) noexcept
{
    return App::Instance().CreateMsg (LinkW(), mid, sz);
}

void ProxyB::Forward (Msg&& msg) noexcept
{
    App::Instance().ForwardMsg (move(msg), LinkW());
}

#ifndef NDEBUG
void ProxyB::CommitMsg (Msg& msg, ostream& os) noexcept
{
    assert (!os.remaining() && "Message body written size does not match requested size");
    assert (msg.Size() == msg.Verify() && "Message body does not match method signature");
}
#endif

//----------------------------------------------------------------------

void Proxy::CreateDestAs (iid_t iid) noexcept
{
    App::Instance().CreateLink (LinkW(), iid);
}

void Proxy::CreateDestWith (iid_t iid, pfn_factory_t fac) noexcept
{
    App::Instance().CreateLinkWith (LinkW(), iid, fac);
}

void Proxy::FreeId (void) noexcept
{
    auto& app = App::Instance();
    if (app.ValidMsgerId (Dest()))
	app.FreeMrid (Dest());
}

//----------------------------------------------------------------------

void Msger::Error (const char* fmt, ...) noexcept // static
{
    va_list args;
    va_start (args, fmt);
    App::Instance().Errorv (fmt, args);
    va_end (args);
}

void Msger::ErrorLibc (const char* f) noexcept // static
{
    Error ("%s: %s", f, strerror(errno));
}

//----------------------------------------------------------------------

Msg::Body::~Body (void) noexcept { wipe(); }

//----------------------------------------------------------------------

Msg::Msg (const Link& l, methodid_t mid, streamsize size, mrid_t extid, fdoffset_t fdo) noexcept
:_method (mid)
,_link (l)
,_extid (extid)
,_fdoffset (fdo)
,_body (Align (size, Alignment::Body))
{
    auto ppade = _body.end();
    _body.shrink (size);
    fill (_body.end(), ppade, 0);	// zero out the alignment padding
}

static streamsize SigelementSize (char c) noexcept
{
    static const struct { char sym; uint8_t sz; } syms[] =
	{{'y',1},{'b',1},{'n',2},{'q',2},{'i',4},{'u',4},{'h',4},{'x',8},{'t',8}};
    for (auto& i : syms)
	if (i.sym == c)
	    return i.sz;
    return 0;
}

static const char* SkipOneSigelement (const char* sig) noexcept
{
    auto parens = 0u;
    do {
	if (*sig == '(')
	    ++parens;
	else if (*sig == ')')
	    --parens;
    } while (*++sig && parens);
    return sig;
}

static streamsize SigelementAlignment (const char* sig) noexcept
{
    auto sz = SigelementSize (*sig);
    if (sz)
	return sz;	// fixed size elements are aligned to size
    if (*sig == 'a' || *sig == 's')
	return 4;
    else if (*sig == '(')
	for (const char* elend = SkipOneSigelement(sig++)-1; sig < elend; sig = SkipOneSigelement(sig))
	    sz = max (sz, SigelementAlignment (sig));
    else assert (!"Invalid signature element while determining alignment");
    return sz;
}

static bool ValidateReadAlign (istream& is, streamsize& sz, streamsize grain) noexcept
{
    if (!is.can_align (grain))
	return false;
    sz += is.alignsz (grain);
    is.align (grain);
    return true;
}

static streamsize ValidateSigelement (istream& is, const char*& sig) noexcept
{
    auto sz = SigelementSize (*sig);
    assert ((sz || *sig == '(' || *sig == 'a' || *sig == 's') && "invalid character in method signature");
    if (sz) {		// Zero size is returned for compound elements, which are structs, arrays, or strings.
	++sig;		// single char element
	if (is.remaining() < sz || !is.aligned(sz))
	    return 0;	// invalid data in buf
	is.skip (sz);
    } else if (*sig == '(') {				// Structs. Scan forward until ')'.
	auto sal = SigelementAlignment (sig);
	if (!ValidateReadAlign (is, sz, sal))
	    return 0;
	++sig;
	for (streamsize ssz; *sig && *sig != ')'; sz += ssz)
	    if (!(ssz = ValidateSigelement (is, sig)))
		return 0;		// invalid data in buf, return 0 as error
	if (!ValidateReadAlign (is, sz, sal))	// align after the struct
	    return 0;
    } else if (*sig == 'a' || *sig == 's') {		// Arrays and strings
	if (is.remaining() < 4 || !is.aligned(4))
	    return 0;
	uint32_t nel; is >> nel;	// number of elements in the array
	sz += sizeof(nel);
	size_t elsz = 1, elal = 4;	// strings are equivalent to "ac"
	if (*sig++ == 'a') {		// arrays are followed by an element sig "a(uqq)"
	    elsz = SigelementSize (*sig);
	    elal = max (SigelementAlignment(sig), 4);
	}
	if (!ValidateReadAlign (is, sz, elal))	// align the beginning of element block
	    return 0;
	if (elsz) {			// optimization for the common case of fixed-element array
	    auto allelsz = elsz*nel;
	    if (is.remaining() < allelsz)
		return 0;
	    is.skip (allelsz);
	    sz += allelsz;
	} else for (auto i = 0u; i < nel; ++i, sz += elsz) {	// read each element
	    auto elsig = sig;		// for each element, pass in the same element sig
	    if (!(elsz = ValidateSigelement (is, elsig)))
		return 0;
	}
	if (sig[-1] == 'a')		// skip the array element sig for arrays; strings do not have one
	    sig = SkipOneSigelement (sig);
	else {				// for strings, verify zero-termination
	    is.unread (1);
	    if (is.readv<char>())
		return 0;
	}
	if (!ValidateReadAlign (is, sz, elal))	// align the end of element block, if element alignment < 4
	    return 0;
    }
    return sz;
}

streamsize Msg::ValidateSignature (istream& is, const char* sig) noexcept // static
{
    streamsize sz = 0;
    while (*sig) {
	auto elsz = ValidateSigelement (is, sig);
	if (!elsz)
	    return 0;
	sz += elsz;
    }
    return sz;
}

} // namespace cwiclo
