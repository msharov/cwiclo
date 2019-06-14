// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "msg.h"
#include "app.h"
#include <stdarg.h>

namespace cwiclo {

methodid_t interface_lookup_method (iid_t iid, const char* __restrict__ mname, size_t mnamesz)
{
    for (methodid_t __restrict__ mid = iid+iid[-1]; mid[0]; mid += mid[0])
	if (equal_n (mname, mnamesz, mid+2, mid[0]-2))
	    return mid+2;
    return nullptr;
}

//----------------------------------------------------------------------

Msg::Msg (const Link& l, methodid_t mid, streamsize size, fdoffset_t fdo)
:_method (mid)
,_link (l)
,_extid (0)
,_fdoffset (fdo)
,_body (ceilg (size, Alignment::Body))
{
    auto ppade = _body.end();
    _body.shrink (size);
    zero_fill (_body.end(), ppade);	// zero out the alignment padding
}

static streamsize sigelement_size (char c)
{
    static const struct { char sym; uint8_t sz; } syms[] = {
	{'y',1},{'c',1},{'b',1},{'q',2},{'n',2},{'u',4},{'i',4},
	{'f',4},{'h',sizeof(Msg::fd_t)},{'x',8},{'t',8},{'d',8}
    };
    for (auto& i : syms)
	if (i.sym == c)
	    return i.sz;
    return 0;
}

static const char* skip_one_sigelement (const char* sig)
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

static streamsize sigelement_alignment (const char* sig)
{
    auto sz = sigelement_size (*sig);
    if (sz)
	return sz;	// fixed size elements are aligned to size
    if (*sig == 'a' || *sig == 's')
	return 4;
    else if (*sig == '(')
	for (const char* elend = skip_one_sigelement(sig++)-1; sig < elend; sig = skip_one_sigelement(sig))
	    sz = max (sz, sigelement_alignment (sig));
    else assert (!"Invalid signature element while determining alignment");
    return sz;
}

static bool validate_read_align (istream& is, streamsize& sz, streamsize grain)
{
    if (!is.can_align (grain))
	return false;
    sz += is.alignsz (grain);
    is.align (grain);
    return true;
}

static streamsize validate_sigelement (istream& is, const char*& sig)
{
    auto sz = sigelement_size (*sig);
    assert ((sz || *sig == '(' || *sig == 'a' || *sig == 's') && "invalid character in method signature");
    if (sz) {		// Zero size is returned for compound elements, which are structs, arrays, or strings.
	++sig;		// single char element
	if (is.remaining() < sz || !is.aligned(sz))
	    return 0;	// invalid data in buf
	is.skip (sz);
    } else if (*sig == '(') {				// Structs. Scan forward until ')'.
	auto sal = sigelement_alignment (sig);
	if (!validate_read_align (is, sz, sal))
	    return 0;
	++sig;
	for (streamsize ssz; *sig && *sig != ')'; sz += ssz)
	    if (!(ssz = validate_sigelement (is, sig)))
		return 0;		// invalid data in buf, return 0 as error
	if (!validate_read_align (is, sz, sal))	// align after the struct
	    return 0;
    } else if (*sig == 'a' || *sig == 's') {		// Arrays and strings
	if (is.remaining() < 4 || !is.aligned(4))
	    return 0;
	uint32_t nel; is >> nel;	// number of elements in the array
	sz += sizeof(nel);
	size_t elsz = 1, elal = 4;	// strings are equivalent to "ac"
	if (*sig++ == 'a') {		// arrays are followed by an element sig "a(uqq)"
	    elsz = sigelement_size (*sig);
	    elal = max (sigelement_alignment(sig), 4);
	}
	if (!validate_read_align (is, sz, elal))	// align the beginning of element block
	    return 0;
	if (elsz) {			// optimization for the common case of fixed-element array
	    auto allelsz = elsz*nel;
	    if (is.remaining() < allelsz)
		return 0;
	    is.skip (allelsz);
	    sz += allelsz;
	} else for (auto i = 0u; i < nel; ++i, sz += elsz) {	// read each element
	    auto elsig = sig;		// for each element, pass in the same element sig
	    if (!(elsz = validate_sigelement (is, elsig)))
		return 0;
	}
	if (sig[-1] == 'a')		// skip the array element sig for arrays; strings do not have one
	    sig = skip_one_sigelement (sig);
	else {				// for strings, verify zero-termination
	    is.unread (1);
	    if (is.read<char>())
		return 0;
	}
	if (!validate_read_align (is, sz, elal))	// align the end of element block, if element alignment < 4
	    return 0;
    }
    return sz;
}

streamsize Msg::validate_signature (istream is, const char* sig) // static
{
    streamsize sz = 0;
    while (*sig) {
	auto elsz = validate_sigelement (is, sig);
	if (!elsz)
	    return 0;
	sz += elsz;
    }
    return sz;
}

//----------------------------------------------------------------------

Msg& ProxyB::create_msg (methodid_t mid, streamsize sz, Msg::fdoffset_t fdo)
{
    return App::instance().create_msg (linkw(), mid, sz, fdo);
}

Msg& ProxyB::create_msg (methodid_t mid, streamsize sz)
{
    return create_msg (mid, sz, Msg::NoFdIncluded);
}

void ProxyB::forward_msg (Msg&& msg)
{
    App::instance().forward_msg (move(msg), linkw());
}

#ifndef NDEBUG
void ProxyB::commit_msg (Msg& msg, ostream& os)
{
    assert (!os.remaining() && "Message body written size does not match requested size");
    assert (msg.size() == msg.verify() && "Message body does not match method signature");
}
#endif

//----------------------------------------------------------------------

void Proxy::create_dest_as (iid_t iid)
{
    App::instance().create_link (linkw(), iid);
}

void Proxy::create_dest_with (iid_t iid, pfn_factory_t fac)
{
    App::instance().create_link_with (linkw(), iid, fac);
}

void Proxy::free_id (void)
{
    auto& app = App::instance();
    if (app.valid_msger_id (dest()))
	app.free_mrid (dest());
}

//----------------------------------------------------------------------

Msger::Msger (void)
: Msger (App::instance().register_singleton_msger (this))
{
}

void Msger::error (const char* fmt, ...) // static
{
    va_list args;
    va_start (args, fmt);
    App::instance().errorv (fmt, args);
    va_end (args);
}

void Msger::error_libc (const char* f) // static
{
    error ("%s: %s", f, strerror(errno));
}

} // namespace cwiclo
