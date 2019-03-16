// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "app.h"
#include <fcntl.h>
#include <signal.h>
#include <time.h>

//{{{ Timer and Signal interfaces --------------------------------------
namespace cwiclo {

DEFINE_INTERFACE (Timer)
DEFINE_INTERFACE (TimerR)
DEFINE_INTERFACE (Signal)

int PTimer::MakeNonblocking (fd_t fd) noexcept // static
{
    auto f = fcntl (fd, F_GETFL);
    return f < 0 ? f : fcntl (fd, F_SETFL, f| O_NONBLOCK| O_CLOEXEC);
}

auto PTimer::Now (void) noexcept -> mstime_t // static
{
    struct timespec t;
    if (0 > clock_gettime (CLOCK_REALTIME, &t))
	return 0;
    return mstime_t(t.tv_nsec) / 1000000 + t.tv_sec * 1000;
}

//}}}-------------------------------------------------------------------
//{{{ App

App*	App::s_pApp		= nullptr;	// static
int	App::s_ExitCode		= EXIT_SUCCESS;	// static
uint32_t App::s_ReceivedSignals	= 0;		// static

App::~App (void) noexcept
{
    // Delete Msgers in reverse order of creation
    for (mrid_t mid = _msgers.size(); mid--;)
	DeleteMsger (mid);
    if (!_errors.empty())
	fprintf (stderr, "Error: %s\n", _errors.c_str());
}

iid_t App::InterfaceByName (const char* iname, streamsize inamesz) noexcept // static
{
    for (auto mii = s_MsgerImpls; mii->iface; ++mii)
	if (equal_n (iname, inamesz, mii->iface, InterfaceNameSize(mii->iface)))
	    return mii->iface;
    return nullptr;
}

//}}}-------------------------------------------------------------------
//{{{ Signal and error handling

#define M(s) BitMask(s)
enum {
    sigset_Die	= M(SIGILL)|M(SIGABRT)|M(SIGBUS)|M(SIGFPE)|M(SIGSYS)|M(SIGSEGV)|M(SIGALRM)|M(SIGXCPU),
    sigset_Quit	= M(SIGINT)|M(SIGQUIT)|M(SIGTERM)|M(SIGPWR),
    sigset_Msg	= sigset_Quit|M(SIGHUP)|M(SIGCHLD)|M(SIGWINCH)|M(SIGURG)|M(SIGXFSZ)|M(SIGUSR1)|M(SIGUSR2)|M(SIGPIPE)
};
#undef M
enum { qc_ShellSignalQuitOffset = 128 };

void App::InstallSignalHandlers (void) noexcept // static
{
    for (auto sig = 0u; sig < NSIG; ++sig) {
	if (GetBit (sigset_Msg, sig))
	    signal (sig, MsgSignalHandler);
	else if (GetBit (sigset_Die, sig))
	    signal (sig, FatalSignalHandler);
    }
}

void App::FatalSignalHandler (int sig) noexcept // static
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

void App::MsgSignalHandler (int sig) noexcept // static
{
    SetBit (s_ReceivedSignals, sig);
    if (GetBit (sigset_Quit, sig)) {
	App::Instance().Quit();
	alarm (1);
    }
}

#ifndef NDEBUG
void App::Errorv (const char* fmt, va_list args) noexcept
{
    bool isFirst = _errors.empty();
    _errors.appendv (fmt, args);
    if (isFirst)
	print_backtrace();
}
#endif

bool App::ForwardError (mrid_t oid, mrid_t eoid) noexcept
{
    auto m = MsgerpById (oid);
    if (!m)
	return false;
    if (m->OnError (eoid, Errors())) {
	_errors.clear();	// error handled; clear message
	return true;
    }
    auto nextoid = m->CreatorId();
    if (nextoid == oid || !ValidMsgerId(nextoid))
	return false;
    return ForwardError (nextoid, oid);
}

//}}}-------------------------------------------------------------------
//{{{ Msger lifecycle

mrid_t App::AllocateMrid (mrid_t creator) noexcept
{
    assert (ValidMsgerId (creator));
    mrid_t id = 0;
    for (; id < _creators.size(); ++id)
	if (_creators[id] == id && !_msgers[id])
	    break;
    if (id > mrid_Last) {
	assert (id <= mrid_Last && "mrid_t address space exhausted; please ensure somebody is freeing them");
	Error ("no more mrids");
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

void App::FreeMrid (mrid_t id) noexcept
{
    assert (ValidMsgerId(id));
    auto m = _msgers[id];
    if (!m && id == _msgers.size()-1) {
	DEBUG_PRINTF ("MsgerId %hu deallocated\n", id);
	_msgers.pop_back();
	_creators.pop_back();
    } else if (auto crid = _creators[id]; crid != id) {
	DEBUG_PRINTF ("MsgerId %hu released\n", id);
	_creators[id] = id;
	if (m) { // act as if the creator was destroyed
	    assert (m->CreatorId() == crid);
	    m->OnMsgerDestroyed (crid);
	}
    }
}

Msger* App::CreateMsgerWith (const Msg::Link& l, iid_t iid [[maybe_unused]], Msger::pfn_factory_t fac) noexcept // static
{
    Msger* r = nullptr;
    if (fac)
	r = (*fac)(l);
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

auto App::CreateMsger (const Msg::Link& l, iid_t iid) noexcept // static
    { return CreateMsgerWith (l, iid, MsgerFactoryFor (iid)); }

Msg::Link& App::CreateLink (Msg::Link& l, iid_t iid) noexcept
{
    assert (l.src <= mrid_Last && "You may only create links originating from an existing Msger");
    assert ((l.dest == mrid_New || l.dest == mrid_Broadcast || ValidMsgerId(l.dest)) && "Invalid link destination requested");
    if (l.dest == mrid_Broadcast)
	return l;
    if (l.dest == mrid_New)
	l.dest = AllocateMrid (l.src);
    if (l.dest < _msgers.size() && !_msgers[l.dest])
	_msgers[l.dest] = CreateMsger (l, iid);
    return l;
}

Msg::Link& App::CreateLinkWith (Msg::Link& l, iid_t iid, Msger::pfn_factory_t fac) noexcept
{
    assert (l.src <= mrid_Last && "You may only create links originating from an existing Msger");
    assert (l.dest == mrid_New && "CreateLinkWith can only be used to create new links");
    l.dest = AllocateMrid (l.src);
    if (l.dest < _msgers.size() && !_msgers[l.dest])
	_msgers[l.dest] = CreateMsgerWith (l, iid, fac);
    return l;
}

void App::DeleteMsger (mrid_t mid) noexcept
{
    assert (ValidMsgerId(mid) && ValidMsgerId(_creators[mid]));
    auto m = exchange (_msgers[mid], nullptr);
    auto crid = _creators[mid];
    if (m && !m->Flag (f_Static)) {
	delete m;
	DEBUG_PRINTF ("Msger %hu deleted\n", mid);
    }

    // Notify creator, if it exists
    if (crid < _msgers.size()) {
	auto cr = _msgers[crid];
	if (cr)
	    cr->OnMsgerDestroyed (mid);
	else // or free mrid if creator is already deleted
	    FreeMrid (mid);
    }

    // Notify connected Msgers of this one's destruction
    for (mrid_t i = 0; i < _creators.size(); ++i)
	if (_creators[i] == mid)
	    FreeMrid (i);
}

void App::DeleteUnusedMsgers (void) noexcept
{
    // A Msger is unused if it has f_Unused flag set and has no pending messages in _outq
    for (auto m : _msgers)
	if (m && m->Flag(f_Unused) && !HasMessagesFor (m->MsgerId()))
	    DeleteMsger (m->MsgerId());
}

//}}}-------------------------------------------------------------------
//{{{ Message loop

void App::MessageLoopOnce (void) noexcept
{
    SwapQueues();
    ProcessInputQueue();
    // End-of-iteration housekeeping
    DeleteUnusedMsgers();
    ForwardReceivedSignals();
}

void App::SwapQueues (void) noexcept
{
    _inq.clear();		// input queue was processed on the last iteration
    _inq.swap (move(_outq));	// output queue now becomes the input queue
}

void App::ProcessInputQueue (void) noexcept
{
    for (auto& msg : _inq) {
	// Dump the message if tracing
	if (DEBUG_MSG_TRACE) {
	    DEBUG_PRINTF ("Msg: %hu -> %hu.%s.%s [%u] = {""{{\n", msg.Src(), msg.Dest(), msg.Interface(), msg.Method(), msg.Size());
	    auto msgbody = msg.Read();
	    hexdump (msgbody.ptr<char>(), msgbody.remaining());
	    DEBUG_PRINTF ("}""}}\n");
	}

	// Create the dispatch range. Broadcast messages go to all, the rest go to one.
	auto mg = 0u, mgend = _msgers.size();
	if (msg.Dest() != mrid_Broadcast) {
	    if (!ValidMsgerId (msg.Dest())) {
		DEBUG_PRINTF ("Error: invalid message destination %hu. Ignoring message.\n", msg.Dest());
		continue; // Error was reported in AllocateMrid
	    }
	    mg = msg.Dest();
	    mgend = mg+1;
	}
	for (; mg < mgend; ++mg) {
	    auto msger = _msgers[mg];
	    if (!msger)
		continue; // errors for msger creation failures were reported in CreateMsger; here just try to continue

	    auto accepted = msger->Dispatch(msg);

	    if (!accepted && msg.Dest() != mrid_Broadcast)
		DEBUG_PRINTF ("Error: message delivered, but not accepted by the destination Msger. Did you forget to add the interface to the Dispatch override?\n");

	    // Check for errors generated during this dispatch
	    if (!Errors().empty() && !ForwardError (mg, mg))
		return Quit (EXIT_FAILURE);
	}
    }
}

void App::ForwardReceivedSignals (void) noexcept
{
    auto oldrs = s_ReceivedSignals;
    if (!oldrs)
	return;
    PSignal psig (mrid_App);
    for (auto i = 0u; i < sizeof(s_ReceivedSignals)*8; ++i)
	if (GetBit (oldrs, i))
	    psig.Signal (i);
    // clear only the signal bits processed, in case new signals arrived during the loop
    s_ReceivedSignals ^= oldrs;
}

App::msgq_t::size_type App::HasMessagesFor (mrid_t mid) const noexcept
{
    App::msgq_t::size_type n = 0;
    for (auto& msg : _outq)
	if (msg.Dest() == mid)
	    ++n;
    return n;
}

//}}}-------------------------------------------------------------------
//{{{ Timers

unsigned App::GetPollTimerList (pollfd* pfd, unsigned pfdsz, int& timeout) const noexcept
{
    // Put all valid fds into the pfd list and calculate the nearest timeout
    // Note that there may be a timeout without any fds
    //
    auto npfd = 0u;
    auto nearest = PTimer::TimerMax;
    for (auto t : _timers) {
	if (t->Cmd() == PTimer::WatchCmd::Stop)
	    continue;
	nearest = min (nearest, t->NextFire());
	if (t->Fd() >= 0) {
	    if (npfd >= pfdsz)
		break;
	    pfd[npfd].fd = t->Fd();
	    pfd[npfd].events = int(t->Cmd());
	    pfd[npfd++].revents = 0;
	}
    }
    if (!_outq.empty())
	timeout = 0;	// do not wait if there are messages to process
    else if (nearest == PTimer::TimerMax)	// wait indefinitely
	timeout = -!!npfd;	// if no fds, then don't wait at all
    else // get current time and compute timeout to nearest
	timeout = max (nearest - PTimer::Now(), 0);
    return npfd;
}

void App::CheckPollTimers (const pollfd* fds) noexcept
{
    // Poll errors are checked for each fd with POLLERR. Other errors are ignored.
    // poll will exit when there are fds available or when the timer expires
    auto now = PTimer::Now();
    const auto* cfd = fds;
    for (auto t : _timers) {
	bool timerExpired = t->NextFire() <= now,
	    hasFd = (t->Fd() >= 0 && t->Cmd() != PTimer::WatchCmd::Stop),
	    fdFired = hasFd && (cfd->revents & (POLLERR| int(t->Cmd())));

	// Log the firing if tracing
	if (DEBUG_MSG_TRACE) {
	    if (timerExpired)
		DEBUG_PRINTF("[T]\tTimer %lu fired at %lu\n", t->NextFire(), now);
	    if (fdFired) {
		DEBUG_PRINTF("[T]\tFile descriptor %d ", cfd->fd);
		if (cfd->revents & POLLIN)	DEBUG_PRINTF("can be read\n");
		if (cfd->revents & POLLOUT)	DEBUG_PRINTF("can be written\n");
		if (cfd->revents & POLLMSG)	DEBUG_PRINTF("has extra data\n");
		if (cfd->revents & POLLERR)	DEBUG_PRINTF("has errors\n");
	    }
	}

	// Firing the timer will remove it (on next idle)
	if (timerExpired || fdFired)
	    t->Fire();
	cfd += hasFd;
    }
}

} // namespace cwiclo
//}}}-------------------------------------------------------------------
