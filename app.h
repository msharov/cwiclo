// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "msg.h"
#include <sys/poll.h>
#include <syslog.h>

//{{{ Debugging macros -------------------------------------------------
namespace cwiclo {

#ifndef NDEBUG
    #define DEBUG_MSG_TRACE	App::instance().flag(App::f_DebugMsgTrace)
    #define DEBUG_PRINTF(...)	do { if (DEBUG_MSG_TRACE) printf (__VA_ARGS__); fflush(stdout); } while (false)
#else
    #define DEBUG_MSG_TRACE	false
    #define DEBUG_PRINTF(...)	do {} while (false)
#endif

//}}}-------------------------------------------------------------------
//{{{ Timer interface

class PTimer : public Proxy {
    DECLARE_INTERFACE (Timer, (watch,"uix"))
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
    using mstime_t = uint64_t;
    static constexpr mstime_t TimerMax = INT64_MAX;
    static constexpr mstime_t TimerNone = UINT64_MAX;
public:
    constexpr	PTimer (mrid_t caller) : Proxy (caller) {}
    void	watch (WatchCmd cmd, fd_t fd, mstime_t timeoutms = TimerNone)
		    { send (m_watch(), cmd, fd, timeoutms); }
    void	stop (void)					{ watch (WatchCmd::Stop, -1, TimerNone); }
    void	timer (mstime_t timeoutms)			{ watch (WatchCmd::Timer, -1, timeoutms); }
    void	wait_read (fd_t fd, mstime_t t = TimerNone)	{ watch (WatchCmd::Read, fd, t); }
    void	wait_write (fd_t fd, mstime_t t = TimerNone)	{ watch (WatchCmd::Write, fd, t); }
    void	wait_rdWr (fd_t fd, mstime_t t = TimerNone)	{ watch (WatchCmd::ReadWrite, fd, t); }

    template <typename O>
    inline static bool dispatch (O* o, const Msg& msg) noexcept {
	if (msg.method() != m_watch())
	    return false;
	auto is = msg.read();
	auto cmd = is.read<WatchCmd>();
	auto fd = is.read<fd_t>();
	auto timer = is.read<mstime_t>();
	o->Timer_watch (cmd, fd, timer);
	return true;
    }
    static int	make_nonblocking (fd_t fd) noexcept;
    static mstime_t now (void) noexcept;
};

//----------------------------------------------------------------------

class PTimerR : public ProxyR {
    DECLARE_INTERFACE (TimerR, (timer,"i"));
public:
    using fd_t = PTimer::fd_t;
public:
    constexpr	PTimerR (const Msg::Link& l)	: ProxyR (l) {}
    void	timer (fd_t fd)			{ send (m_timer(), fd); }

    template <typename O>
    inline static bool dispatch (O* o, const Msg& msg) noexcept {
	if (msg.method() != m_timer())
	    return false;
	o->TimerR_timer (msg.read().read<fd_t>());
	return true;
    }
};

//}}}-------------------------------------------------------------------
//{{{ Signal interface

class PSignal : public Proxy {
    DECLARE_INTERFACE (Signal, (signal,"i"));
public:
    constexpr	PSignal (mrid_t caller)	: Proxy (caller, mrid_Broadcast) {}
    void	signal (int sig)	{ send (m_signal(), sig); }

    template <typename O>
    inline static bool dispatch (O* o, const Msg& msg) noexcept {
	if (msg.method() != m_signal())
	    return false;
	o->Signal_signal (msg.read().read<int>());
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
    static auto&	instance (void)			{ return *s_pApp; }
    static void		install_signal_handlers (void) noexcept;
    void		process_args (argc_t, argv_t)	{ }
    inline int		run (void) noexcept;
    Msg::Link&		create_link (Msg::Link& l, iid_t iid) noexcept;
    Msg::Link&		create_link_with (Msg::Link& l, iid_t iid, Msger::pfn_factory_t fac) noexcept;
    inline Msg&		create_msg (Msg::Link& l, methodid_t mid, streamsize size, Msg::fdoffset_t fdo = Msg::NoFdIncluded) noexcept
			    { return _outq.emplace_back (create_link (l,interface_of_method(mid)),mid,size,fdo); }
    inline void		forward_msg (Msg&& msg, Msg::Link& l) noexcept
			    { _outq.emplace_back (move(msg), create_link(l,msg.interface())); }
    static iid_t	interface_by_name (const char* iname, streamsize inamesz) noexcept;
    msgq_t::size_type	has_messages_for (mrid_t mid) const noexcept;
    constexpr auto	has_timers (void) const		{ return _timers.size(); }
    bool		valid_msger_id (mrid_t id)const	{ assert (_msgers.size() == _creators.size()); return id <= _msgers.size(); }
    constexpr void	quit (void)			{ set_flag (f_Quitting); }
    constexpr void	quit (int ec)			{ s_exit_code = ec; quit(); }
    auto		exit_code (void) const		{ return s_exit_code; }
    auto&		errors (void) const		{ return _errors; }
    void		free_mrid (mrid_t id) noexcept;
    void		message_loop_once (void) noexcept;
    void		delete_msger (mrid_t mid) noexcept;
    unsigned		get_poll_timer_list (pollfd* pfd, unsigned pfdsz, int& timeout) const noexcept;
    void		check_poll_timers (const pollfd* fds) noexcept;
    bool		forward_error (mrid_t oid, mrid_t eoid) noexcept;
#ifdef NDEBUG
    void		errorv (const char* fmt, va_list args) noexcept	{ _errors.appendv (fmt, args); }
#else
    void		errorv (const char* fmt, va_list args) noexcept;
#endif
protected:
    inline		App (void) noexcept;
			~App (void) noexcept override;
    [[noreturn]] static void fatal_signal_handler (int sig) noexcept;
    static void		msg_signal_handler (int sig) noexcept;
private:
    //{{{2 MsgerFactoryMap ---------------------------------------------
    // Maps a factory to an interface
    struct MsgerFactoryMap {
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
			    { App::instance().add_timer (this); }
			~Timer (void) noexcept override
			    { App::instance().remove_timer (this); }
	bool		dispatch (Msg& msg) noexcept override
			    { return PTimer::dispatch(this,msg) || Msger::dispatch(msg); }
	inline void	Timer_watch (PTimer::WatchCmd cmd, PTimer::fd_t fd, mstime_t timeoutms) noexcept;
	constexpr void	stop (void)		{ set_flag (f_Unused); _cmd = PTimer::WatchCmd::Stop; _fd = -1; _nextfire = PTimer::TimerNone; }
	void		fire (void)		{ _reply.timer (_fd); stop(); }
	auto		fd (void) const		{ return _fd; }
	auto		cmd (void) const	{ return _cmd; }
	auto		next_fire (void) const	{ return _nextfire; }
	auto		poll_mask (void) const	{ return _cmd; }
    public:
	PTimer::mstime_t	_nextfire;
	PTimerR			_reply;
	PTimer::WatchCmd	_cmd;
	PTimer::fd_t		_fd;
    };
    //}}}2--------------------------------------------------------------
private:
    mrid_t		allocate_mrid (mrid_t creator) noexcept;
    auto		msgerp_by_id (mrid_t id)	{ return _msgers[id]; }
    inline static auto	msger_factory_for (iid_t id) noexcept;
    [[nodiscard]] inline static Msger*	create_msger_with (const Msg::Link& l, iid_t iid, Msger::pfn_factory_t fac) noexcept;
    [[nodiscard]] inline static auto	create_msger (const Msg::Link& l, iid_t iid) noexcept;
    inline void		process_input_queue (void) noexcept;
    inline void		delete_unused_msgers (void) noexcept;
    inline void		forward_received_signals (void) noexcept;
    void		add_timer (Timer* t)	{ _timers.push_back (t); }
    void		remove_timer (Timer* t)	{ remove (_timers, t); }
    inline void		run_timers (void) noexcept;
private:
    msgq_t		_outq;
    msgq_t		_inq;
    vector<Msger*>	_msgers;
    vector<Timer*>	_timers;
    vector<mrid_t>	_creators;
    string		_errors;
    static App*		s_pApp;
    static int		s_exit_code;
    static uint32_t	s_received_signals;
    static const MsgerFactoryMap s_msger_factories[];
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
    _msgers.push_back (this);
    _creators.push_back (mrid_App);
}

int App::run (void) noexcept
{
    if (!errors().empty())	// Check for errors generated in ctor and ProcessArgs
	return EXIT_FAILURE;
    while (!flag (f_Quitting)) {
	message_loop_once();
	run_timers();
    }
    return exit_code();
}

void App::run_timers (void) noexcept
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

void App::Timer::Timer_watch (PTimer::WatchCmd cmd, PTimer::fd_t fd, mstime_t timeoutms) noexcept
{
    _cmd = cmd;
    set_unused (_cmd == PTimer::WatchCmd::Stop);
    _fd = fd;
    _nextfire = timeoutms + (timeoutms <= PTimer::TimerMax ? PTimer::now() : PTimer::TimerNone);
}

//}}}-------------------------------------------------------------------
//{{{ main template

template <typename A>
inline int mainT (typename A::argc_t argc, typename A::argv_t argv)
{
    A::install_signal_handlers();
    auto& a = A::instance();
    a.process_args (argc, argv);
    return a.run();
}
#define CWICLO_MAIN(A)	\
int main (A::argc_t argc, A::argv_t argv) \
    { return mainT<A> (argc, argv); }

#define BEGIN_MSGERS	const App::MsgerFactoryMap App::s_msger_factories[] = {
#define REGISTER_MSGER(iface,mgtype)	{ P##iface::interface(), &Msger::factory<mgtype> },
#define END_MSGERS	{nullptr,nullptr}};

#define BEGIN_CWICLO_APP(A)	\
    CWICLO_MAIN(A)		\
    BEGIN_MSGERS
#define END_CWICLO_APP		END_MSGERS

} // namespace cwiclo
//}}}-------------------------------------------------------------------
