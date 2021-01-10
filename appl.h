// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "xcom.h"
#include <sys/poll.h>

//{{{ Debugging macros -------------------------------------------------
namespace cwiclo {

#ifndef NDEBUG
    #define DEBUG_MSG_TRACE	AppL::instance().flag(AppL::f_DebugMsgTrace)
    #define DEBUG_PRINTF(...)	do { if (DEBUG_MSG_TRACE) printf (__VA_ARGS__); fflush(stdout); } while (false)
#else
    #define DEBUG_MSG_TRACE	false
    #define DEBUG_PRINTF(...)	do {} while (false)
#endif

//}}}-------------------------------------------------------------------
//{{{ Timer interface

class PTimer : public Proxy {
    DECLARE_INTERFACE (Proxy, Timer, (watch,"uix")(timer,"i"))
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
    static mstime_t now (void) { return now_milliseconds(); }

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
public:
    class Reply : public Proxy::Reply {
    public:
	constexpr	Reply (Msg::Link l)	: Proxy::Reply (l) {}
	void		timer (fd_t fd) const	{ send (m_timer(), fd); }

	template <typename O>
	inline static constexpr bool dispatch (O* o, const Msg& msg) {
	    if (msg.method() != m_timer())
		return false;
	    o->Timer_timer (msg.read().read<fd_t>());
	    return true;
	}
    };
};

//}}}-------------------------------------------------------------------
//{{{ Signal interface

class PSignal : public Proxy {
    #define SIGNATURE_Signal_Info	"(iiii)"
    DECLARE_INTERFACE (Proxy, Signal, (signal,SIGNATURE_Signal_Info))
public:
    struct Info {
	int32_t	sig;
	int32_t	status;
	int32_t	pid;
	int32_t	uid;
    };
public:
    constexpr	PSignal (mrid_t caller)	: Proxy (caller, mrid_Broadcast) {}
    void	signal (const Info& si) const	{ send (m_signal(), si); }

    template <typename O>
    inline static constexpr bool dispatch (O* o, const Msg& msg) {
	if (msg.method() != m_signal())
	    return false;
	o->Signal_signal (msg.read().read<Info>());
	return true;
    }
    using Reply = PSignal;
};

//}}}-------------------------------------------------------------------
//{{{ AppL

class AppL : public Msger {
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
    void		create_method_dest (methodid_t mid, Msg::Link l);
    void		create_dest_with (iid_t iid, Msger::pfn_factory_t fac, Msg::Link l);
    inline Msg&		create_msg (Msg::Link l, methodid_t mid, streamsize size, Msg::fdoffset_t fdo = Msg::NoFdIncluded)
			    { create_method_dest (mid,l); return _outq.emplace_back (l,mid,size,fdo); }
    inline Msg&		create_msg (Msg::Link l, methodid_t mid, Msg::Body&& body, Msg::fdoffset_t fdo = Msg::NoFdIncluded, extid_t extid = 0)
			    { create_method_dest (mid,l); return _outq.emplace_back (l,mid,move(body),fdo,extid); }
    mrid_t		register_singleton_msger (Msger* m);
    msgq_t::size_type	has_messages_for (mrid_t mid) const;
    Msg*		has_outq_msg (methodid_t mid, Msg::Link l);
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
			AppL (void);
			~AppL (void) override;
    [[noreturn]] static void fatal_signal_handler (int sig);
    static void		msg_signal_handler (int sig);
public:
    //{{{2 MsgerFactoryMap ---------------------------------------------
    // Maps a factory to an interface
    struct MsgerFactoryMap {
	iid_t			iface;
	Msger::pfn_factory_t	factory;
    };
    //}}}2--------------------------------------------------------------
    //{{{2 Timer
    friend class Timer;
    class Timer : public Msger {
	IMPLEMENT_INTERFACES_I (Msger, (PTimer),)
    public:
	explicit	Timer (Msg::Link l)
			    : Msger(l),_nextfire(PTimer::TimerNone),_cmd(),_fd(-1)
			    { AppL::instance().add_timer (this); }
			~Timer (void) override;
	inline void	Timer_watch (PTimer::WatchCmd cmd, fd_t fd, mstime_t timeoutms);
	constexpr void	stop (void)		{ set_flag (f_Unused); _cmd = PTimer::WatchCmd::Stop; _fd = -1; _nextfire = PTimer::TimerNone; }
	void		fire (void)		{ PTimer::Reply(creator_link()).timer (_fd); stop(); }
	auto		fd (void) const		{ return _fd; }
	auto		cmd (void) const	{ return _cmd; }
	auto		next_fire (void) const	{ return _nextfire; }
	auto		poll_mask (void) const	{ return _cmd; }
    public:
	PTimer::mstime_t	_nextfire;
	PTimer::WatchCmd	_cmd;
	fd_t			_fd;
    };
    //}}}2--------------------------------------------------------------
private:
    auto		msgerp_by_id (mrid_t id)	{ return _msgers[id]; }
    inline static auto	msger_factory_for (iid_t id);
    [[nodiscard]] inline static Msger*	create_msger_with (Msg::Link l, iid_t iid, Msger::pfn_factory_t fac);
    [[nodiscard]] inline static auto	create_msger (Msg::Link l, iid_t iid);
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
    static AppL*	s_pApp;
    static int		s_exit_code;
    static uint32_t	s_received_signals;
    static const MsgerFactoryMap* s_msger_factories;
};

//}}}-------------------------------------------------------------------
//{{{ CWICLO_APP_L

//{{{2 Helper templates
namespace {

template <typename A>
inline static int Tmain (typename A::argc_t argc, typename A::argv_t argv)
{
    A::install_signal_handlers();
    auto& a = A::instance();
    a.process_args (argc, argv);
    return a.run();
}

template <typename M>
static constexpr auto get_msger_factory_maps (AppL::MsgerFactoryMap* m, Msger::pfn_factory_t pfac)
{
    constexpr const auto ni = M::n_interfaces();
    iid_t mias [ni] = {};
    M::get_interfaces (begin(mias));
    for (auto i = 0u; i < ni; ++i, ++m) {
	m->iface = mias[i];
	m->factory = pfac;
    }
    return m;
}

} // namespace
//}}}2
//{{{2 Helper macros

#define CWICLO_MAIN(A)	\
int main (A::argc_t argc, A::argv_t argv) \
    { return Tmain<A> (argc, argv); }

#define GENERATE_MSGER_INTERFACE_COUNTER(arg,msger)	arg msger::n_interfaces()
#define GENERATE_MSGER_FACTORY_MAP_ENTRY(arg,msger)	arg = get_msger_factory_maps<msger>(arg,&msger::factory<msger>);

#define GENERATE_MSGER_FACTORY_MAP(msgers,nullfac)	\
namespace {						\
    constexpr auto generate_msger_factory_map (void) {	\
	constexpr auto ni = 1 SEQ_FOR_EACH(msgers,+,GENERATE_MSGER_INTERFACE_COUNTER);\
	struct { AppL::MsgerFactoryMap m [ni]; } r = {};	\
	auto m = begin(r.m);				\
	SEQ_FOR_EACH(msgers,m,GENERATE_MSGER_FACTORY_MAP_ENTRY)\
	m->factory = nullfac;				\
	return r;					\
    }							\
    static constexpr auto s_factory_map = generate_msger_factory_map();\
}							\
const AppL::MsgerFactoryMap* AppL::s_msger_factories = s_factory_map.m;

//}}}2

#define CWICLO_APP_L(A,msgers)	\
    CWICLO_MAIN(A)		\
    GENERATE_MSGER_FACTORY_MAP(msgers,nullptr)

} // namespace cwiclo
//}}}-------------------------------------------------------------------
