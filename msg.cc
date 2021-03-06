// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "msg.h"
#include "appl.h"
#include <stdarg.h>

namespace cwiclo {

iid_t interface_lookup (const iid_t* __restrict__ il, const char* __restrict__ is, size_t islen)
{
    for (auto i = il; *i; ++i)
	if (equal_n (*i, interface_name_size(*i), is, islen))
	    return *i;
    return nullptr;
}

methodid_t interface_lookup_method (iid_t __restrict__ iid, const char* __restrict__ mname, size_t mnamesz)
{
    methodid_t __restrict__ mid = interface_first_method (iid);
    for (uint8_t nextm; (nextm = method_next_offset (mid)); mid += nextm)
	if (equal_n (mname, mnamesz, mid, method_name_size(mid)))
	    return mid;
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
    if (sz) {
	//
	// Fixed size element
	//
	++sig;
	if (is.remaining() < sz || !is.aligned(sz))
	    return 0;	// invalid data in buf
	is.skip (sz);

    } else if (*sig == '(') {
	//
	// Struct. Scan forward until ')'.
	//
	auto sal = sigelement_alignment (sig);
	if (!validate_read_align (is, sz, sal))
	    return 0;
	++sig;		// Add up all the members of the struct
	for (streamsize ssz; *sig && *sig != ')'; sz += ssz)
	    if (!(ssz = validate_sigelement (is, sig)))
		return 0;		// invalid data in buf, return 0 as error
	assert (*sig == ')' && "unterminated struct in signature");
	++sig;
	if (!validate_read_align (is, sz, sal))	// align after the struct
	    return 0;

    } else if (*sig == 'a' || *sig == 's') {
	//
	// Arrays and strings
	//
	if (is.remaining() < 4 || !is.aligned(4))
	    return 0;
	uint32_t nel; is >> nel;	// number of elements in the array
	sz += sizeof(nel);

	size_t elsz = 1, elal = 4;	// strings are equivalent to "ac"
	if (*sig++ == 'a') {		// arrays are followed by an element sig "a(uqq)"
	    elsz = sigelement_size (*sig);
	    elal = max (sigelement_alignment(sig), 4);
	}

	// align the beginning of element block
	if (!validate_read_align (is, sz, elal))
	    return 0;

	if (elsz) {
	    //
	    // optimization for the common case of fixed-size element array
	    //
	    auto allelsz = elsz*nel;
	    if (is.remaining() < allelsz)
		return 0;
	    is.skip (allelsz);
	    sz += allelsz;
	} else {
	    //
	    // read elements individually
	    //
	    for (auto i = 0u; i < nel; ++i, sz += elsz) {
		auto elsig = sig;		// for each element, pass in the same element sig
		if (!(elsz = validate_sigelement (is, elsig)))
		    return 0;
	    }
	}

	if (sig[-1] == 'a')		// skip the array element sig for arrays; strings do not have one
	    sig = skip_one_sigelement (sig);
	else {				// for strings, verify zero-termination
	    is.unread (1);
	    if (is.read<char>())
		return 0;
	}

	// align the end of element block, if element alignment < 4
	if (!validate_read_align (is, sz, elal))
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

Msg& IDispatch::create_msg (methodid_t mid, streamsize sz, Msg::fdoffset_t fdo) const
    { return AppL::instance().create_msg (link(), mid, sz, fdo); }
Msg& IDispatch::create_msg (methodid_t mid, streamsize sz) const
    { return create_msg (mid, sz, Msg::NoFdIncluded); }
Msg& IDispatch::create_msg (methodid_t imethod, Msg::Body&& body, Msg::fdoffset_t fdo, extid_t extid) const
    { return AppL::instance().create_msg (link(), imethod, move(body), fdo, extid); }

Msg* IDispatch::get_outgoing_msg (methodid_t imethod) const
    { return AppL::instance().has_outq_msg (imethod, link()); }

Msg& IDispatch::recreate_msg (methodid_t imethod, streamsize sz) const
{
    if (auto msg = get_outgoing_msg (imethod); msg) {
	msg->resize_body (sz);
	return *msg;
    }
    return create_msg (imethod, sz);
}

Msg& IDispatch::recreate_msg (methodid_t imethod, Msg::Body&& body) const
{
    if (auto msg = get_outgoing_msg (imethod); msg) {
	msg->replace_body (move(body));
	return *msg;
    }
    return create_msg (imethod, move(body));
}

//----------------------------------------------------------------------

Interface::Interface (mrid_t from)
: IDispatch (from, allocate_id (from))
{
}

mrid_t Interface::allocate_id (mrid_t src) // static
    { return AppL::instance().allocate_mrid (src); }
void Interface::free_id (void)
    { AppL::instance().free_mrid (IDispatch::set_dest (mrid_Broadcast)); }

void Interface::create_dest_for (iid_t iid) const
{
    AppL::instance().create_method_dest (interface_first_method(iid), link());
}

void Interface::create_dest_with (pfn_factory_t fac, iid_t iid) const
{
    AppL::instance().create_dest_with (iid, fac, link());
}

//----------------------------------------------------------------------

Msger::Msger (void)
: Msger (AppL::instance().register_singleton_msger (this))
{
}

void Msger::error (const char* fmt, ...) // static
{
    va_list args;
    va_start (args, fmt);
    AppL::instance().errorv (fmt, args);
    va_end (args);
}

void Msger::error_libc (const char* f) // static
{
    error ("%s: %s", f, strerror(errno));
}

} // namespace cwiclo
