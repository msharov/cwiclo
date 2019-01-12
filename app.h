// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "msg.h"
#include <sys/poll.h>
#include <syslog.h>

//{{{ Debugging macros -------------------------------------------------
namespace cwiclo {

#ifndef NDEBUG
    #define DEBUG_MSG_TRACE	App::Instance().Flag(App::f_DebugMsgTrace)
    #define DEBUG_PRINTF(...)	do { if (DEBUG_MSG_TRACE) printf (__VA_ARGS__); fflush(stdout); } while (false)
#else
    #define DEBUG_MSG_TRACE	false
    #define DEBUG_PRINTF(...)	do {} while (false)
#endif

//}}}-------------------------------------------------------------------
//{{{ Timer interface

class PTimer : public Proxy {
    DECLARE_INTERFACE (Timer, (Watch,"uix"))
public:
    enum class WatchCmd : uint32_t {
	Stop		= 0,
	Read		= POLLIN,
	Write		= POLLOUT,
	ReadWrite	= Read| Write,
	Timer		= POLLMSG,
	ReadTimer	= Read| Timer,
	WriteTimer	= Write| Timer,
	ReadWriteTimer	= ReadWrite| Timer
    };
    using fd_t = int32_t;
    using mstime_t = uint64_t;
    static constexpr mstime_t TimerMax = INT64_MAX;
    static constexpr mstime_t TimerNone = UINT64_MAX;
public:
		PTimer (mrid_t caller) : Proxy (caller) {}
    void	Watch (WatchCmd cmd, fd_t fd, mstime_t timeoutms = TimerNone)
		    { Send (M_Watch(), cmd, fd, timeoutms); }
    void	Stop (void)					{ Watch (WatchCmd::Stop, -1, TimerNone); }
    void	Timer (mstime_t timeoutms)			{ Watch (WatchCmd::Timer, -1, timeoutms); }
    void	WaitRead (fd_t fd, mstime_t t = TimerNone)	{ Watch (WatchCmd::Read, fd, t); }
    void	WaitWrite (fd_t fd, mstime_t t = TimerNone)	{ Watch (WatchCmd::Write, fd, t); }
    void	WaitRdWr (fd_t fd, mstime_t t = TimerNone)	{ Watch (WatchCmd::ReadWrite, fd, t); }

    template <typename O>
    inline static bool Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() != M_Watch())
	    return false;
	auto is = msg.Read();
	auto cmd = is.readv<WatchCmd>();
	auto fd = is.readv<fd_t>();
	auto timer = is.readv<mstime_t>();
	o->Timer_Watch (cmd, fd, timer);
	return true;
    }
    static mstime_t Now (void) noexcept;
};

//----------------------------------------------------------------------

class PTimerR : public ProxyR {
    DECLARE_INTERFACE (TimerR, (Timer,"i"));
public:
    using fd_t = PTimer::fd_t;
public:
		PTimerR (const Msg::Link& l)	: ProxyR (l) {}
    void	Timer (fd_t fd)			{ Send (M_Timer(), fd); }

    template <typename O>
    inline static bool Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() != M_Timer())
	    return false;
	o->TimerR_Timer (msg.Read().readv<fd_t>());
	return true;
    }
};

//}}}-------------------------------------------------------------------
//{{{ Signal interface

class PSignal : public Proxy {
    DECLARE_INTERFACE (Signal, (Signal,"i"));
public:
		PSignal (mrid_t caller)	: Proxy (caller, mrid_Broadcast) {}
    void	Signal (int sig)	{ Send (M_Signal(), sig); }

    template <typename O>
    inline static bool Dispatch (O* o, const Msg& msg) noexcept {
	if (msg.Method() != M_Signal())
	    return false;
	o->Signal_Signal (msg.Read().readv<int>());
	return true;
    }
};

//}}}-------------------------------------------------------------------
//{{{ App

class App : public Msger {
public:
    using argc_t	= int;
    using argv_t	= char* const*;
    using mstime_t	= PTimer::mstime_t;
    using msgq_t	= vector<Msg>;
    enum { f_Quitting = Msger::f_Last, f_DebugMsgTrace, f_Last };
public:
    static auto&	Instance (void)			{ return *s_pApp; }
    static void		InstallSignalHandlers (void) noexcept;
    void		ProcessArgs (argc_t, argv_t)	{ }
    inline int		Run (void) noexcept;
    Msg::Link&		CreateLink (Msg::Link& l, iid_t iid) noexcept;
    Msg::Link&		CreateLinkWith (Msg::Link& l, iid_t iid, Msger::pfn_factory_t fac) noexcept;
    inline Msg&		CreateMsg (Msg::Link& l, methodid_t mid, streamsize size, mrid_t extid = 0, Msg::fdoffset_t fdo = Msg::NoFdIncluded) noexcept;
    inline void		ForwardMsg (Msg&& msg, Msg::Link& l) noexcept;
    static iid_t	InterfaceByName (const char* iname, streamsize inamesz) noexcept;
    msgq_t::size_type	HasMessagesFor (mrid_t mid) const noexcept;
    auto		HasTimers (void) const		{ return _timers.size(); }
    bool		ValidMsgerId (mrid_t id) const	{ assert (_msgers.size() == _creators.size()); return id <= _msgers.size(); }
    void		Quit (void)			{ SetFlag (f_Quitting); }
    void		Quit (int ec)			{ s_ExitCode = ec; Quit(); }
    auto&		Errors (void) const		{ return _errors; }
    void		FreeMrid (mrid_t id) noexcept;
    void		MessageLoopOnce (void) noexcept;
    void		DeleteMsger (mrid_t mid) noexcept;
    unsigned		GetPollTimerList (pollfd* pfd, unsigned pfdsz, int& timeout) const noexcept;
    void		CheckPollTimers (const pollfd* fds) noexcept;
    bool		ForwardError (mrid_t oid, mrid_t eoid) noexcept;
#ifdef NDEBUG
    void		Errorv (const char* fmt, va_list args) noexcept	{ _errors.appendv (fmt, args); }
#else
    void		Errorv (const char* fmt, va_list args) noexcept;
#endif
protected:
    inline		App (void) noexcept;
			~App (void) noexcept override;
    [[noreturn]] static void FatalSignalHandler (int sig) noexcept;
    static void		MsgSignalHandler (int sig) noexcept;
private:
    //{{{2 MsgerImplements ---------------------------------------------
    // Maps a factory to an interface
    struct MsgerImplements {
	iid_t			iface;
	Msger::pfn_factory_t	factory;
    };
    //}}}2--------------------------------------------------------------
public:
    //{{{2 Timer
    friend class Timer;
    class Timer : public Msger {
    public:
	explicit	Timer (const Msg::Link& l) : Msger(l),_nextfire(PTimer::TimerNone),_reply(l),_cmd(),_fd(-1)
			    { App::Instance().AddTimer (this); }
			~Timer (void) noexcept override
			    { App::Instance().RemoveTimer (this); }
	bool		Dispatch (Msg& msg) noexcept override
			    { return PTimer::Dispatch(this,msg) || Msger::Dispatch(msg); }
	inline void	Timer_Watch (PTimer::WatchCmd cmd, PTimer::fd_t fd, mstime_t timeoutms) noexcept;
	void		Stop (void)		{ SetFlag (f_Unused); _cmd = PTimer::WatchCmd::Stop; _fd = -1; _nextfire = PTimer::TimerNone; }
	void		Fire (void)		{ _reply.Timer (_fd); Stop(); }
	auto		Fd (void) const		{ return _fd; }
	auto		Cmd (void) const	{ return _cmd; }
	auto		NextFire (void) const	{ return _nextfire; }
	auto		PollMask (void) const	{ return _cmd; }
    public:
	PTimer::mstime_t	_nextfire;
	PTimerR			_reply;
	PTimer::WatchCmd	_cmd;
	PTimer::fd_t		_fd;
    };
    //}}}2--------------------------------------------------------------
private:
    mrid_t		AllocateMrid (mrid_t creator) noexcept;
    auto		MsgerpById (mrid_t id)	{ return _msgers[id]; }
    inline static auto	MsgerFactoryFor (iid_t id) {
			    auto mii = s_MsgerImpls;
			    while (mii->iface && mii->iface != id)
				++mii;
			    return mii->factory;
			}
    inline void		SwapQueues (void) noexcept;
    [[nodiscard]] inline static Msger*	CreateMsgerWith (const Msg::Link& l, iid_t iid, Msger::pfn_factory_t fac) noexcept;
    [[nodiscard]] inline static auto	CreateMsger (const Msg::Link& l, iid_t iid) noexcept;
    inline void		ProcessInputQueue (void) noexcept;
    inline void		DeleteUnusedMsgers (void) noexcept;
    inline void		ForwardReceivedSignals (void) noexcept;
    void		AddTimer (Timer* t)	{ _timers.push_back(t); }
    void		RemoveTimer (Timer* t)	{ remove_if (_timers, [&](auto i){ return i == t; }); }
    inline void		RunTimers (void) noexcept;
private:
    msgq_t		_outq;
    msgq_t		_inq;
    vector<Msger*>	_msgers;
    vector<Timer*>	_timers;
    vector<mrid_t>	_creators;
    string		_errors;
    static App*		s_pApp;
    static const MsgerImplements s_MsgerImpls[];
    static int		s_ExitCode;
    static uint32_t	s_ReceivedSignals;
};

//----------------------------------------------------------------------

App::App (void) noexcept
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
    _msgers.emplace_back (this);
    _creators.push_back (mrid_App);
}

int App::Run (void) noexcept
{
    if (!Errors().empty())	// Check for errors generated in ctor and ProcessArgs
	return EXIT_FAILURE;
    while (!Flag (f_Quitting)) {
	MessageLoopOnce();
	RunTimers();
    }
    return s_ExitCode;
}

void App::RunTimers (void) noexcept
{
    auto ntimers = HasTimers();
    if (!ntimers || Flag(f_Quitting)) {
	if (_outq.empty()) {
	    DEBUG_PRINTF ("Warning: ran out of packets. Quitting.\n");
	    SetFlag (f_Quitting);	// running out of packets is usually not what you want, but not exactly an error
	}
	return;
    }

    // Populate the fd list and find the nearest timer
    pollfd fds [ntimers];
    int timeout;
    auto nfds = GetPollTimerList (fds, ntimers, timeout);
    if (!nfds && !timeout) {
	if (_outq.empty()) {
	    DEBUG_PRINTF ("Warning: ran out of packets. Quitting.\n");
	    SetFlag (f_Quitting);	// running out of packets is usually not what you want, but not exactly an error
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
    CheckPollTimers (fds);
}

void App::Timer::Timer_Watch (PTimer::WatchCmd cmd, PTimer::fd_t fd, mstime_t timeoutms) noexcept
{
    _cmd = cmd;
    SetFlag (f_Unused, _cmd == PTimer::WatchCmd::Stop);
    _fd = fd;
    _nextfire = timeoutms + (timeoutms <= PTimer::TimerMax ? PTimer::Now() : PTimer::TimerNone);
}

Msg& App::CreateMsg (Msg::Link& l, methodid_t mid, streamsize size, mrid_t extid, Msg::fdoffset_t fdo) noexcept
{
    return _outq.emplace_back (CreateLink(l,InterfaceOfMethod(mid)),mid,size,extid,fdo);
}

void App::ForwardMsg (Msg&& msg, Msg::Link& l) noexcept
{
    _outq.emplace_back (move(msg), CreateLink(l,msg.Interface()));
}

//}}}-------------------------------------------------------------------
//{{{ main template

template <typename A>
inline int Tmain (typename A::argc_t argc, typename A::argv_t argv)
{
    A::InstallSignalHandlers();
    auto& a = A::Instance();
    a.ProcessArgs (argc, argv);
    return a.Run();
}
#define CwicloMain(A)	\
int main (A::argc_t argc, A::argv_t argv) \
    { return Tmain<A> (argc, argv); }

#define BEGIN_MSGERS	const App::MsgerImplements App::s_MsgerImpls[] = {
#define REGISTER_MSGER(iface,mgtype)	{ P##iface::Interface(), &Msger::Factory<mgtype> },
#define END_MSGERS	{nullptr,nullptr}};

#define BEGIN_CWICLO_APP(A)	\
    CwicloMain(A)		\
    BEGIN_MSGERS
#define END_CWICLO_APP		END_MSGERS

} // namespace cwiclo
//}}}-------------------------------------------------------------------
