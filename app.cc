// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "app.h"
#include <fcntl.h>
#include <signal.h>
#include <time.h>

//{{{ Timer and Signal interfaces --------------------------------------
namespace cwiclo {

DEFINE_INTERFACE (Timer)
DEFINE_INTERFACE (TimerR)
DEFINE_INTERFACE (Signal)

int PTimer::make_nonblocking (fd_t fd) noexcept // static
{
    auto f = fcntl (fd, F_GETFL);
    return f < 0 ? f : fcntl (fd, F_SETFL, f| O_NONBLOCK| O_CLOEXEC);
}

auto PTimer::now (void) noexcept -> mstime_t // static
{
    struct timespec t;
    if (0 > clock_gettime (CLOCK_REALTIME, &t))
	return 0;
    return mstime_t(t.tv_nsec) / 1000000 + t.tv_sec * 1000;
}

//}}}-------------------------------------------------------------------
//{{{ App

App*	App::s_pApp		= nullptr;	// static
int	App::s_exit_code	= EXIT_SUCCESS;	// static
uint32_t App::s_received_signals = 0;		// static

App::~App (void) noexcept
{
    // Delete Msgers in reverse order of creation
    for (mrid_t mid = _msgers.size(); mid--;)
	delete_msger (mid);
    if (!_errors.empty())
	fprintf (stderr, "Error: %s\n", _errors.c_str());
}

iid_t App::interface_by_name (const char* iname, streamsize inamesz) noexcept // static
{
    for (auto mii = s_msger_factories; mii->iface; ++mii)
	if (equal_n (iname, inamesz, mii->iface, interface_name_size (mii->iface)))
	    return mii->iface;
    return nullptr;
}

//}}}-------------------------------------------------------------------
//{{{ Signal and error handling

#define M(s) bit_mask(s)
enum {
    sigset_Die	= M(SIGILL)|M(SIGABRT)|M(SIGBUS)|M(SIGFPE)|M(SIGSYS)|M(SIGSEGV)|M(SIGALRM)|M(SIGXCPU),
    sigset_Quit	= M(SIGINT)|M(SIGQUIT)|M(SIGTERM)|M(SIGPWR),
    sigset_Msg	= sigset_Quit|M(SIGHUP)|M(SIGCHLD)|M(SIGWINCH)|M(SIGURG)|M(SIGXFSZ)|M(SIGUSR1)|M(SIGUSR2)|M(SIGPIPE)
};
#undef M
enum { qc_ShellSignalQuitOffset = 128 };

void App::install_signal_handlers (void) noexcept // static
{
    for (auto sig = 0u; sig < NSIG; ++sig) {
	if (get_bit (sigset_Msg, sig))
	    signal (sig, msg_signal_handler);
	else if (get_bit (sigset_Die, sig))
	    signal (sig, fatal_signal_handler);
    }
}

void App::fatal_signal_handler (int sig) noexcept // static
{
    static atomic_flag doubleSignal = ATOMIC_FLAG_INIT;
    if (!doubleSignal.test_and_set (memory_order::relaxed)) {
	alarm (1);
	fprintf (stderr, "[S] Error: %s\n", strsignal(sig));
	#ifndef NDEBUG
	    print_backtrace();
	#endif
	exit (qc_ShellSignalQuitOffset+sig);
    }
    _Exit (qc_ShellSignalQuitOffset+sig);
}

void App::msg_signal_handler (int sig) noexcept // static
{
    set_bit (s_received_signals, sig);
    if (get_bit (sigset_Quit, sig)) {
	App::instance().quit();
	alarm (1);
    }
}

#ifndef NDEBUG
void App::errorv (const char* fmt, va_list args) noexcept
{
    bool isFirst = _errors.empty();
    _errors.appendv (fmt, args);
    if (isFirst)
	print_backtrace();
}
#endif

bool App::forward_error (mrid_t oid, mrid_t eoid) noexcept
{
    auto m = msgerp_by_id (oid);
    if (!m)
	return false;
    if (m->on_error (eoid, errors())) {
	_errors.clear();	// error handled; clear message
	return true;
    }
    auto nextoid = m->creator_id();
    if (nextoid == oid || !valid_msger_id(nextoid))
	return false;
    return forward_error (nextoid, oid);
}

//}}}-------------------------------------------------------------------
//{{{ Msger lifecycle

mrid_t App::allocate_mrid (mrid_t creator) noexcept
{
    assert (valid_msger_id (creator));
    mrid_t id = 0;
    for (; id < _creators.size(); ++id)
	if (_creators[id] == id && !_msgers[id])
	    break;
    if (id > mrid_Last) {
	assert (id <= mrid_Last && "mrid_t address space exhausted; please ensure somebody is freeing them");
	error ("no more mrids");
	return id;
    } else if (id >= _creators.size()) {
	_msgers.push_back (nullptr);
	_creators.push_back (creator);
    } else {
	assert (!_msgers[id]);
	_creators[id] = creator;
    }
    return id;
}

void App::free_mrid (mrid_t id) noexcept
{
    assert (valid_msger_id(id));
    auto m = _msgers[id];
    if (!m && id == _msgers.size()-1) {
	DEBUG_PRINTF ("mrid %hu deallocated\n", id);
	_msgers.pop_back();
	_creators.pop_back();
    } else if (auto crid = _creators[id]; crid != id) {
	DEBUG_PRINTF ("mrid %hu released\n", id);
	_creators[id] = id;
	if (m) { // act as if the creator was destroyed
	    assert (m->creator_id() == crid);
	    m->on_msger_destroyed (crid);
	}
    }
}

Msger* App::create_msger_with (const Msg::Link& l, iid_t iid [[maybe_unused]], Msger::pfn_factory_t fac) noexcept // static
{
    Msger* r = fac ? (*fac)(l) : nullptr;
    #ifndef NDEBUG	// Log failure to create in debug mode
	if (!r && (!iid || !iid[0] || iid[iid[-1]-1] != 'R')) { // reply messages do not recreate dead Msgers
	    if (!fac) {
		DEBUG_PRINTF ("Error: no factory registered for interface %s\n", iid ? iid : "(iid_null)");
		assert (!"Unable to find factory for the given interface. You must register a Msger for every interface you use using REGISTER_MSGER in the BEGIN_MSGER/END_MSGER block.");
	    } else {
		DEBUG_PRINTF ("Error: failed to create Msger for interface %s\n", iid ? iid : "(iid_null)");
		assert (!"Failed to create Msger for the given destination. Msger constructors are not allowed to fail or throw.");
	    }
	} else
	    DEBUG_PRINTF ("Created Msger %hu as %s\n", l.dest, iid);
    #endif
    return r;
}

auto App::msger_factory_for (iid_t id) noexcept // static
{
    auto mii = s_msger_factories;
    while (mii->iface && mii->iface != id)
	++mii;
    return mii->factory;
}

auto App::create_msger (const Msg::Link& l, iid_t iid) noexcept // static
    { return create_msger_with (l, iid, msger_factory_for (iid)); }

Msg::Link& App::create_link (Msg::Link& l, iid_t iid) noexcept
{
    assert (valid_msger_id (l.src) && "You may only create links originating from an existing Msger");
    assert ((l.dest == mrid_New || l.dest == mrid_Broadcast || valid_msger_id(l.dest)) && "Invalid link destination requested");
    if (l.dest == mrid_Broadcast)
	return l;
    if (l.dest == mrid_New)
	l.dest = allocate_mrid (l.src);
    if (l.dest < _msgers.size() && !_msgers[l.dest])
	_msgers[l.dest] = create_msger (l, iid);
    return l;
}

Msg::Link& App::create_link_with (Msg::Link& l, iid_t iid, Msger::pfn_factory_t fac) noexcept
{
    assert (valid_msger_id (l.src) && "You may only create links originating from an existing Msger");
    assert (l.dest == mrid_New && "create_link_with can only be used to create new links");
    l.dest = allocate_mrid (l.src);
    if (l.dest < _msgers.size() && !_msgers[l.dest])
	_msgers[l.dest] = create_msger_with (l, iid, fac);
    return l;
}

void App::delete_msger (mrid_t mid) noexcept
{
    assert (valid_msger_id(mid) && valid_msger_id(_creators[mid]));
    auto m = exchange (_msgers[mid], nullptr);
    auto crid = _creators[mid];
    if (m && !m->flag (f_Static)) {
	delete m;
	DEBUG_PRINTF ("Msger %hu deleted\n", mid);
    }

    // Notify creator, if it exists
    if (crid < _msgers.size()) {
	if (auto cr = _msgers[crid]; cr)
	    cr->on_msger_destroyed (mid);
	else // or free mrid if creator is already deleted
	    free_mrid (mid);
    }

    // Notify connected Msgers of this one's destruction
    for (auto i = 0u; i < _creators.size(); ++i)
	if (_creators[i] == mid)
	    free_mrid (i);
}

void App::delete_unused_msgers (void) noexcept
{
    // A Msger is unused if it has f_Unused flag set and has no pending messages in _outq
    for (auto m : _msgers)
	if (m && m->flag (f_Unused) && !has_messages_for (m->msger_id()))
	    delete_msger (m->msger_id());
}

//}}}-------------------------------------------------------------------
//{{{ Message loop

void App::message_loop_once (void) noexcept
{
    _inq.clear();		// input queue was processed on the last iteration
    _inq.swap (move(_outq));	// output queue now becomes the input queue

    process_input_queue();
    delete_unused_msgers();
    forward_received_signals();
}

void App::process_input_queue (void) noexcept
{
    for (auto& msg : _inq) {
	// Dump the message if tracing
	if (DEBUG_MSG_TRACE) {
	    DEBUG_PRINTF ("Msg: %hu -> %hu.%s.%s [%u] = {""{{\n", msg.src(), msg.dest(), msg.interface(), msg.method(), msg.size());
	    hexdump (msg.read());
	    DEBUG_PRINTF ("}""}}\n");
	}

	// Create the dispatch range. Broadcast messages go to all, the rest go to one.
	auto mg = 0u, mgend = _msgers.size();
	if (msg.dest() != mrid_Broadcast) {
	    if (!valid_msger_id (msg.dest())) {
		DEBUG_PRINTF ("Error: invalid message destination %hu. Ignoring message.\n", msg.dest());
		continue; // Error was reported in allocate_mrid
	    }
	    mg = msg.dest();
	    mgend = mg+1;
	}
	for (; mg < mgend; ++mg) {
	    auto msger = _msgers[mg];
	    if (!msger)
		continue; // errors for msger creation failures were reported in create_msger; here just try to continue

	    auto accepted = msger->dispatch (msg);

	    if (!accepted && msg.dest() != mrid_Broadcast)
		DEBUG_PRINTF ("Error: message delivered, but not accepted by the destination Msger.\nDid you forget to add the interface to the dispatch override?\n");

	    // Check for errors generated during this dispatch
	    if (!errors().empty() && !forward_error (mg, mg))
		return quit (EXIT_FAILURE);
	}
    }
}

void App::forward_received_signals (void) noexcept
{
    auto oldrs = s_received_signals;
    if (!oldrs)
	return;
    PSignal psig (mrid_App);
    for (auto i = 0u; i < sizeof(s_received_signals)*8; ++i)
	if (get_bit (oldrs, i))
	    psig.signal (i);
    // clear only the signal bits processed, in case new signals arrived during the loop
    s_received_signals ^= oldrs;
}

App::msgq_t::size_type App::has_messages_for (mrid_t mid) const noexcept
{
    return count_if (_outq, [=](auto& msg){ return msg.dest() == mid; });
}

//}}}-------------------------------------------------------------------
//{{{ Timers

unsigned App::get_poll_timer_list (pollfd* pfd, unsigned pfdsz, int& timeout) const noexcept
{
    // Put all valid fds into the pfd list and calculate the nearest timeout
    // Note that there may be a timeout without any fds
    //
    auto npfd = 0u;
    auto nearest = PTimer::TimerMax;
    for (auto t : _timers) {
	if (t->cmd() == PTimer::WatchCmd::Stop)
	    continue;
	nearest = min (nearest, t->next_fire());
	if (t->fd() >= 0) {
	    if (npfd >= pfdsz)
		break;
	    pfd[npfd].fd = t->fd();
	    pfd[npfd].events = int(t->cmd());
	    pfd[npfd++].revents = 0;
	}
    }
    if (!_outq.empty())
	timeout = 0;	// do not wait if there are messages to process
    else if (nearest == PTimer::TimerMax)	// wait indefinitely
	timeout = -!!npfd;	// if no fds, then don't wait at all
    else // get current time and compute timeout to nearest
	timeout = max (nearest - PTimer::now(), 0);
    return npfd;
}

void App::check_poll_timers (const pollfd* fds) noexcept
{
    // Poll errors are checked for each fd with POLLERR. Other errors are ignored.
    // poll will exit when there are fds available or when the timer expires
    auto now = PTimer::now();
    const auto* cfd = fds;
    for (auto t : _timers) {
	bool expired = t->next_fire() <= now,
	    hasfd = (t->fd() >= 0 && t->cmd() != PTimer::WatchCmd::Stop),
	    fdon = hasfd && (cfd->revents & (POLLERR| int(t->cmd())));

	// Log the firing if tracing
	if (DEBUG_MSG_TRACE) {
	    if (expired)
		DEBUG_PRINTF("[T]\tTimer %lu fired at %lu\n", t->next_fire(), now);
	    if (fdon) {
		DEBUG_PRINTF("[T]\tFile descriptor %d ", cfd->fd);
		if (cfd->revents & POLLIN)	DEBUG_PRINTF("can be read\n");
		if (cfd->revents & POLLOUT)	DEBUG_PRINTF("can be written\n");
		if (cfd->revents & POLLMSG)	DEBUG_PRINTF("has extra data\n");
		if (cfd->revents & POLLERR)	DEBUG_PRINTF("has errors\n");
	    }
	}

	// Firing the timer will remove it (on next idle)
	if (expired || fdon)
	    t->fire();
	cfd += hasfd;
    }
}

} // namespace cwiclo
//}}}-------------------------------------------------------------------
