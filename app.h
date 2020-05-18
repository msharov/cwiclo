// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "msg.h"
#include <sys/poll.h>

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
	Timer		= POLLPRI,
	ReadTimer	= Read| Timer,
	WriteTimer	= Write| Timer,
	ReadWriteTimer	= ReadWrite| Timer
    };
    using mstime_t = uint64_t;
    static constexpr mstime_t TimerMax = INT64_MAX;
    static constexpr mstime_t TimerNone = UINT64_MAX;
public:
    explicit	PTimer (mrid_t caller) : Proxy (caller) {}
    void	watch (WatchCmd cmd, fd_t fd, mstime_t timeoutms = TimerNone) const;
    void	stop (void) const				{ watch (WatchCmd::Stop, -1, TimerNone); }
    void	timer (mstime_t timeoutms) const		{ watch (WatchCmd::Timer, -1, timeoutms); }
    void	wait_read (fd_t fd, mstime_t t=TimerNone) const	{ watch (WatchCmd::Read, fd, t); }
    void	wait_write (fd_t fd, mstime_t t=TimerNone)const	{ watch (WatchCmd::Write, fd, t); }
    void	wait_rdWr (fd_t fd, mstime_t t=TimerNone) const	{ watch (WatchCmd::ReadWrite, fd, t); }

    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() != m_watch())
	    return false;
	auto is = msg.read();
	auto cmd = is.read<WatchCmd>();
	auto fd = is.read<fd_t>();
	auto timer = is.read<mstime_t>();
	o->Timer_watch (cmd, fd, timer);
	return true;
    }
    static int	make_nonblocking (fd_t fd);
    static int	make_blocking (fd_t fd);
    static mstime_t now (void);
};

//----------------------------------------------------------------------

class PTimerR : public ProxyR {
    DECLARE_INTERFACE (TimerR, (timer,"i"));
public:
    using fd_t = PTimer::fd_t;
public:
    constexpr	PTimerR (const Msg::Link& l)	: ProxyR (l) {}
    void	timer (fd_t fd) const		{ send (m_timer(), fd); }

    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
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
    void	signal (int sig) const	{ send (m_signal(), sig); }

    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
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
    static void		install_signal_handlers (void);
    void		process_args (argc_t, argv_t)	{ }
    int			run (void);
    void		create_dest (iid_t iid, const Msg::Link& l);
    void		create_dest_with (iid_t iid, Msger::pfn_factory_t fac, const Msg::Link& l);
    inline Msg&		create_msg (const Msg::Link& l, methodid_t mid, streamsize size, Msg::fdoffset_t fdo = Msg::NoFdIncluded)
			    { create_dest (interface_of_method(mid),l); return _outq.emplace_back (l,mid,size,fdo); }
    inline Msg&		create_msg (const Msg::Link& l, methodid_t mid, Msg::Body&& body, Msg::fdoffset_t fdo = Msg::NoFdIncluded, extid_t extid = 0)
			    { create_dest (interface_of_method(mid),l); return _outq.emplace_back (l,mid,move(body),fdo,extid); }
    mrid_t		register_singleton_msger (Msger* m);
    static iid_t	interface_by_name (const char* iname, streamsize inamesz);
    msgq_t::size_type	has_messages_for (mrid_t mid) const;
    Msg*		has_outq_msg (methodid_t mid, const Msg::Link& l);
    constexpr auto	has_timers (void) const		{ return _timers.size(); }
    bool		valid_msger_id (mrid_t id)const	{ assert (_msgers.size() == _creators.size()); return id <= _msgers.size(); }
    constexpr void	quit (void)			{ set_flag (f_Quitting); }
    constexpr void	quit (int ec)			{ s_exit_code = ec; quit(); }
    auto		exit_code (void) const		{ return s_exit_code; }
    auto&		errors (void) const		{ return _errors; }
    mrid_t		allocate_mrid (mrid_t creator);
    void		free_mrid (mrid_t id);
    void		message_loop_once (void);
    void		delete_msger (mrid_t mid);
    unsigned		get_poll_timer_list (pollfd* pfd, unsigned pfdsz, int& timeout) const;
    void		check_poll_timers (const pollfd* fds);
    bool		forward_error (mrid_t oid, mrid_t eoid);
#ifdef NDEBUG
    void		errorv (const char* fmt, va_list args)	{ _errors.appendv (fmt, args); }
#else
    void		errorv (const char* fmt, va_list args);
#endif
protected:
			App (void);
			~App (void) override;
    [[noreturn]] static void fatal_signal_handler (int sig);
    static void		msg_signal_handler (int sig);
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
	explicit	Timer (const Msg::Link& l)
			    : Msger(l),_nextfire(PTimer::TimerNone),_reply(l),_cmd(),_fd(-1)
			    { App::instance().add_timer (this); }
			~Timer (void) override;
	bool		dispatch (Msg& msg) override;
	inline void	Timer_watch (PTimer::WatchCmd cmd, PTimer::fd_t fd, mstime_t timeoutms);
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
    auto		msgerp_by_id (mrid_t id)	{ return _msgers[id]; }
    inline static auto	msger_factory_for (iid_t id);
    [[nodiscard]] inline static Msger*	create_msger_with (const Msg::Link& l, iid_t iid, Msger::pfn_factory_t fac);
    [[nodiscard]] inline static auto	create_msger (const Msg::Link& l, iid_t iid);
    inline void		process_input_queue (void);
    inline void		delete_unused_msgers (void);
    inline void		forward_received_signals (void);
    void		add_timer (Timer* t)	{ _timers.push_back (t); }
    void		remove_timer (Timer* t)	{ remove (_timers, t); }
    void		run_timers (void);
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
#define REGISTER_MSGER(proxy,mgtype)	{ proxy::interface(), &Msger::factory<mgtype> },
#define END_MSGERS	{nullptr,nullptr}};

#define BEGIN_CWICLO_APP(A)	\
    CWICLO_MAIN(A)		\
    BEGIN_MSGERS
#define END_CWICLO_APP		END_MSGERS

} // namespace cwiclo
//}}}-------------------------------------------------------------------
