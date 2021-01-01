// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "app.h"
#include <signal.h>
#include <sys/wait.h>

//{{{ Timer and Signal interfaces --------------------------------------
namespace cwiclo {

void PTimer::watch (WatchCmd cmd, fd_t fd, mstime_t timeoutms) const
    { send (m_watch(), cmd, fd, timeoutms); }

//----------------------------------------------------------------------

App::Timer::~Timer (void)
    { App::instance().remove_timer (this); }

void App::Timer::Timer_watch (PTimer::WatchCmd cmd, PTimer::fd_t fd, mstime_t timeoutms)
{
    _cmd = cmd;
    set_unused (_cmd == PTimer::WatchCmd::Stop);
    _fd = fd;
    _nextfire = timeoutms + (timeoutms <= PTimer::TimerMax ? PTimer::now() : PTimer::TimerNone);
}

IMPLEMENT_INTERFACES_D (App::Timer)

//}}}-------------------------------------------------------------------
//{{{ App

App*	App::s_pApp		= nullptr;	// static
int	App::s_exit_code	= EXIT_SUCCESS;	// static
uint32_t App::s_received_signals = 0;		// static

//----------------------------------------------------------------------

App::App (void)
: Msger (mrid_App)
,_outq()
,_inq()
,_msgers()
,_timers()
,_creators()
,_errors()
{
    assert (!s_pApp && "there must be only one App object");
    s_pApp = this;
    register_singleton_msger (this);
}

App::~App (void)
{
    // Delete Msgers in reverse order of creation
    for (mrid_t mid = _msgers.size(); mid--;)
	delete_msger (mid);
    if (!_errors.empty())
	fprintf (stderr, "Error: %s\n", _errors.c_str());
}

//}}}-------------------------------------------------------------------
//{{{ Signal and error handling

#define M(s) bit_mask(s)
enum {
    sigset_Die	= M(SIGILL)|M(SIGABRT)|M(SIGBUS)|M(SIGFPE)|M(SIGSYS)|M(SIGSEGV)|M(SIGALRM)|M(SIGXCPU),
    sigset_Quit	= M(SIGINT)|M(SIGQUIT)|M(SIGTERM),
    sigset_Msg	= sigset_Quit|M(SIGHUP)|M(SIGCHLD)|M(SIGWINCH)|M(SIGURG)|M(SIGXFSZ)|M(SIGUSR1)|M(SIGUSR2)|M(SIGPIPE)
};
#undef M
enum { qc_ShellSignalQuitOffset = 128 };

void App::install_signal_handlers (void) // static
{
    for (auto sig = 0u; sig < NSIG; ++sig) {
	if (get_bit (sigset_Msg, sig))
	    signal (sig, msg_signal_handler);
	else if (get_bit (sigset_Die, sig))
	    signal (sig, fatal_signal_handler);
    }
}

void App::fatal_signal_handler (int sig) // static
{
    static atomic_flag doubleSignal = ATOMIC_FLAG_INIT;
    if (!doubleSignal.test_and_set (memory_order::relaxed)) {
	#ifdef NDEBUG
	    alarm (1);
	#endif
	psignal (sig, "[S] Error");
	#ifndef NDEBUG
	    print_backtrace();
	#endif
	exit (qc_ShellSignalQuitOffset+sig);
    }
    _Exit (qc_ShellSignalQuitOffset+sig);
}

void App::msg_signal_handler (int sig) // static
{
    set_bit (s_received_signals, sig);
    if (get_bit (sigset_Quit, sig)) {
	App::instance().quit();
	#ifdef NDEBUG
	    alarm (1);
	#endif
    }
}

#ifndef NDEBUG
void App::errorv (const char* fmt, va_list args)
{
    bool is_first = _errors.empty();
    _errors.appendv (fmt, args);
    DEBUG_PRINTF ("[E] Error: %s\n", _errors.c_str());
    if (is_first)
	print_backtrace();
}
#endif

bool App::forward_error (mrid_t oid, mrid_t eoid)
{
    auto m = msgerp_by_id (oid);
    if (!m)
	return false;
    if (m->on_error (eoid, errors())) {
	DEBUG_PRINTF ("[E] Error handled.\n");
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

mrid_t App::allocate_mrid (mrid_t creator)
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
    } else if (id == _creators.size()) {
	_msgers.push_back (nullptr);
	_creators.push_back (creator);
    } else
	_creators[id] = creator;
    return id;
}

void App::free_mrid (mrid_t id)
{
    if (!valid_msger_id (id))
	return;
    auto m = _msgers[id];
    if (!m && id == _msgers.size()-1) {
	DEBUG_PRINTF ("[M] mrid %hu deallocated\n", id);
	_msgers.pop_back();
	_creators.pop_back();
    } else if (auto crid = _creators[id]; crid != id) {
	DEBUG_PRINTF ("[M] mrid %hu released\n", id);
	_creators[id] = id;
	if (m) { // act as if the creator was destroyed
	    assert (m->creator_id() == crid);
	    m->on_msger_destroyed (crid);
	}
    }
}

mrid_t App::register_singleton_msger (Msger* m)
{
    auto id = allocate_mrid (mrid_App);
    if (id <= mrid_Last) {
	_msgers[id] = m;
	DEBUG_PRINTF ("[M] Created Msger %hu singleton\n", id);
    }
    return id;
}

Msger* App::create_msger_with (Msg::Link l, iid_t iid [[maybe_unused]], Msger::pfn_factory_t fac) // static
{
    Msger* r = fac ? (*fac)(l) : nullptr;
    #ifndef NDEBUG	// Log failure to create in debug mode
	if (!r && (!iid || !iid[0])) {
	    if (!fac) {
		DEBUG_PRINTF ("[E] No factory registered for interface %s\n", iid ? iid : "(iid_null)");
		assert (!"Unable to find factory for the given interface. You must add a Msger to CWICLO_APP for every interface you use.");
	    } else {
		DEBUG_PRINTF ("[E] Failed to create Msger for interface %s\n", iid ? iid : "(iid_null)");
		assert (!"Failed to create Msger for the given destination. Msger constructors are not allowed to fail or throw.");
	    }
	} else
	    DEBUG_PRINTF ("[M] Created Msger %hu as %s\n", l.dest, iid);
    #endif
    return r;
}

auto App::msger_factory_for (iid_t id) // static
{
    auto mii = s_msger_factories;
    while (mii->iface && mii->iface != id)
	++mii;
    return mii->factory;
}

auto App::create_msger (Msg::Link l, iid_t iid) // static
    { return create_msger_with (l, iid, msger_factory_for (iid)); }

void App::create_method_dest (methodid_t mid, Msg::Link l)
{
    assert (valid_msger_id (l.src) && "You may only create links originating from an existing Msger");
    if (l.dest < _msgers.size() && !_msgers[l.dest]) {
	if (_creators[l.dest] == l.src)
	    _msgers[l.dest] = create_msger (l, interface_of_method(mid));
	else // messages for a deleted Msger can arrive if the sender was not yet aware of the deletion, in another process, for example, where the notification had not arrived. Condition logged, but is not usually an error.
	    DEBUG_PRINTF ("Warning: dead destination Msger %hu can only be resurrected by creator %hu, not %hu.\n", l.dest, _creators[l.dest], l.src);
    }
}

void App::create_dest_with (iid_t iid, Msger::pfn_factory_t fac, Msg::Link l)
{
    assert (valid_msger_id (l.src) && "You may only create links originating from an existing Msger");
    if (l.dest < _msgers.size() && !_msgers[l.dest])
	_msgers[l.dest] = create_msger_with (l, iid, fac);
}

void App::delete_msger (mrid_t mid)
{
    assert (valid_msger_id(mid) && valid_msger_id(_creators[mid]));
    auto m = exchange (_msgers[mid], nullptr);
    auto crid = _creators[mid];
    if (m && !m->flag (f_Static)) {
	delete m;
	DEBUG_PRINTF ("[M] Msger %hu deleted\n", mid);
    }

    // Notify Msgers created by this one of its destruction
    for (auto i = _creators.size(); i--;)
	if (_creators[i] == mid)
	    free_mrid (i);

    // Notify creator, if it exists
    if (crid < _msgers.size() && _msgers[crid])
	_msgers[crid]->on_msger_destroyed (mid);
    else if (crid != mid) // or free mrid if creator is already deleted
	free_mrid (mid);
}

void App::delete_unused_msgers (void)
{
    // A Msger is unused if it has f_Unused flag set and has no pending messages in _outq
    for (auto m : _msgers)
	if (m && m->flag (f_Unused) && !has_messages_for (m->msger_id()))
	    delete_msger (m->msger_id());
}

//}}}-------------------------------------------------------------------
//{{{ Message loop

int App::run (void)
{
    if (!errors().empty())	// Check for errors generated in ctor and ProcessArgs
	return EXIT_FAILURE;
    while (!flag (f_Quitting)) {
	message_loop_once();
	run_timers();
    }
    return exit_code();
}

void App::message_loop_once (void)
{
    _inq.clear();	// input queue was processed on the last iteration
    _inq.swap (_outq);	// output queue now becomes the input queue

    process_input_queue();
    delete_unused_msgers();
    forward_received_signals();
}

void App::process_input_queue (void)
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
		DEBUG_PRINTF ("[E] Invalid message destination %hu. Ignoring message.\n", msg.dest());
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
		DEBUG_PRINTF ("[E] Message delivered, but not accepted by the destination Msger.\nDid you forget to add the interface to IMPLEMENT_INTERFACES?\n");

	    // Check for errors generated during this dispatch
	    if (!errors().empty() && !forward_error (mg, mg))
		return quit (EXIT_FAILURE);
	}
    }
}

void App::run_timers (void)
{
    auto ntimers = has_timers();
    if (!ntimers || flag (f_Quitting)) {
	if (_outq.empty()) {
	    DEBUG_PRINTF ("Warning: ran out of packets. Quitting.\n");
	    quit();	// running out of packets is usually not what you want, but not exactly an error
	}
	return;
    }

    // Populate the fd list and find the nearest timer
    pollfd fds [ntimers];
    int timeout;
    auto nfds = get_poll_timer_list (fds, ntimers, timeout);
    if (!nfds && !timeout) {
	if (_outq.empty()) {
	    DEBUG_PRINTF ("Warning: ran out of packets. Quitting.\n");
	    quit();	// running out of packets is usually not what you want, but not exactly an error
	}
	return;
    }

    // And wait
    if (DEBUG_MSG_TRACE) {
	DEBUG_PRINTF ("----------------------------------------------------------------------\n");
	if (timeout > 0)
	    DEBUG_PRINTF ("[I] Waiting for %d ms ", timeout);
	else if (timeout < 0)
	    DEBUG_PRINTF ("[I] Waiting indefinitely ");
	else if (!timeout)
	    DEBUG_PRINTF ("[I] Checking ");
	DEBUG_PRINTF ("%u file descriptors from %u timers\n", nfds, ntimers);
    }

    // And poll
    poll (fds, nfds, timeout);

    // Then, check timers for expiration
    check_poll_timers (fds);
}

void App::forward_received_signals (void)
{
    auto oldrs = s_received_signals;
    if (!oldrs)
	return;
    PSignal psig (mrid_App);
    for (auto i = 0u; i < sizeof(s_received_signals)*8; ++i) {
	if (!get_bit (oldrs, i))
	    continue;
	PSignal::Info si = {};
	if ((si.sig = i) == SIGCHLD) {
	    if (0 >= (si.pid = waitpid (-1, &si.status, WNOHANG)))
		continue;
	    --i;	// multiple children may exit
	}
	psig.signal (si);
    }
    // clear only the signal bits processed, in case new signals arrived during the loop
    s_received_signals ^= oldrs;
}

App::msgq_t::size_type App::has_messages_for (mrid_t mid) const
{
    return count_if (_outq, [=](auto& msg){ return msg.dest() == mid; });
}

Msg* App::has_outq_msg (methodid_t mid, Msg::Link l)
{
    eachfor (mi, _outq)
	if (mi->method() == mid && mi->link() == l)
	    return mi;
    return nullptr;
}

//}}}-------------------------------------------------------------------
//{{{ Timers

unsigned App::get_poll_timer_list (pollfd* pfd, unsigned pfdsz, int& timeout) const
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

void App::check_poll_timers (const pollfd* fds)
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
		if (cfd->revents & POLLPRI)	DEBUG_PRINTF("has extra data\n");
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
