// This file is part of the cwiclo project
//
// Copyright (c) 2021 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#pragma once
#include "appl.h"
#include "xcom.h"

namespace cwiclo {

class Extern;

class App : public AppL {
    IMPLEMENT_INTERFACES_I (AppL,,(ITimer))
public:
    enum { f_SocketActivated = base_class_t::f_Last, f_ListenWhenEmpty, f_Last };
public:
    static auto&	instance (void)	{ return static_cast<App&>(base_class_t::instance()); }
    static auto		imports (void)	{ return s_imports; }
    static auto		exports (void)	{ return s_exports; }
    void		init (argc_t argc, argv_t argv);
    bool		on_error (mrid_t eid, const string& errmsg) override;
    void		on_msger_destroyed (mrid_t mid) override;
    iid_t		extern_interface_by_name (const char* is, size_t islen) const;
    Extern*		extern_by_id (mrid_t eid) const;
    Extern*		create_extern_dest_for (iid_t iid);
protected:
			App (void)	: base_class_t(),_isock(),_esock(),_socknames() {}
			friend class ITimer::Reply;
    void		Timer_timer (fd_t fd);
    bool		accept_socket_activation (void);
    void		add_listen_socket (fd_t fd, const char* sockname = "", const char* sockfile = "");
    void		create_listen_socket (const char* path, const char* sockname = "");
    virtual void	accept_socket (fd_t fd, const char* sockname);
    static iid_t	listed_interface_by_name (const iid_t* il, const char* is, size_t islen);
private:
    //{{{ IListener
    class IListener : public ITimer {
    public:
	explicit	IListener (fd_t fd, const char* sockname = "", const char* sockfile = "")
			    : ITimer (mrid_App),_sockfd(fd),_sockname(sockname),_sockfile (sockfile) {}
			~IListener (void);
	auto		sockfd (void) const	{ return _sockfd; }
	auto		sockname (void) const	{ return _sockname; }
	auto&		sockfile (void) const	{ return _sockfile; }
	void		wait_read (void) const	{ ITimer::wait_read (sockfd()); }
    private:
	fd_t		_sockfd;
	const char*	_sockname;
	string		_sockfile;
    };
    //}}}
private:
    vector<IExtern>	_isock;
    vector<IListener>	_esock;
    string		_socknames;
private:
    static const iid_t*	s_imports;
    static const iid_t*	s_exports;
};

//{{{ CWICLO_APP -------------------------------------------------------

//{{{2 Imports list

#define GENERATE_IFACE_INTERFACE_COUNTER(arg,iface)	arg iface::n_interfaces()
#define GENERATE_IFACE_GET_INTERFACES(arg,iface)	arg = iface::get_interfaces(arg);

#define GENERATE_IMPORTS_LIST(interfaces)		\
namespace {						\
    constexpr auto generate_imports_list (void) {	\
	constexpr auto ni = 1 SEQ_FOR_EACH(interfaces,+,GENERATE_IFACE_INTERFACE_COUNTER);\
	struct { iid_t ia [ni]; } r = {};		\
	auto i = begin(r.ia);				\
	SEQ_FOR_EACH(interfaces,i,GENERATE_IFACE_GET_INTERFACES)\
	*i = nullptr;					\
	return r;					\
    }							\
    static constexpr auto s_imports_list = generate_imports_list();\
}							\
const iid_t* App::s_imports = s_imports_list.ia;

//}}}2
//{{{2 Exports list

#define GENERATE_EXPORTS_LIST(interfaces)		\
namespace {						\
    constexpr auto generate_exports_list (void) {	\
	constexpr auto ni = 1 SEQ_FOR_EACH(interfaces,+,GENERATE_IFACE_INTERFACE_COUNTER);\
	struct { iid_t ia [ni]; } r = {};		\
	auto i = begin(r.ia);				\
	SEQ_FOR_EACH(interfaces,i,GENERATE_IFACE_GET_INTERFACES)\
	*i = nullptr;					\
	return r;					\
    }							\
    static constexpr auto s_exports_list = generate_exports_list();\
}							\
const iid_t* App::s_exports = s_exports_list.ia;

//}}}2

#define CWICLO_APP(A,msgers,imports,exports)	\
    CWICLO_MAIN(A)				\
    GENERATE_MSGER_FACTORY_MAP((AppL::Timer)(Extern) msgers,&COMRelay::factory<COMRelay>)\
    GENERATE_IMPORTS_LIST((ICOM) imports)	\
    GENERATE_EXPORTS_LIST(exports)

//}}}-------------------------------------------------------------------

} // namespace cwiclo
