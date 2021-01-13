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
    IMPLEMENT_INTERFACES_I (AppL,,(PTimer))
public:
    enum { f_ListenWhenEmpty = base_class_t::f_Last, f_Last };
public:
    static auto&	instance (void)	{ return static_cast<App&>(base_class_t::instance()); }
    static auto		imports (void)	{ return s_imports; }
    static auto		exports (void)	{ return s_exports; }
    int			run (void);
    bool		on_error (mrid_t eid, const string& errmsg) override;
    void		on_msger_destroyed (mrid_t mid) override;
    iid_t		extern_interface_by_name (const char* is, size_t islen) const;
    Extern*		extern_by_id (mrid_t eid) const;
    Extern*		create_extern_dest_for (iid_t iid);
protected:
			App (void)	: base_class_t(),_isock(),_esock() {}
			friend class PTimer::Reply;
    inline void		Timer_timer (fd_t fd);
    static iid_t	listed_interface_by_name (const iid_t* il, const char* is, size_t islen);
    bool		accept_socket_activation (void);
    void		create_extern_socket (const char* path);
    void		add_extern_socket (fd_t fd, const char* sockname = "");
    void		add_extern_connection (fd_t fd);
private:
    //{{{ PListener
    class PListener : public PTimer {
    public:
	explicit	PListener (void)	: PTimer (mrid_App),_sockfd(-1),_sockname() {}
			PListener (fd_t fd, const char* sockname)
			    : PTimer (mrid_App),_sockfd(fd),_sockname()
			    { if (auto snl = strlen(sockname); snl) _sockname.assign (sockname, snl); }
			~PListener (void);
	auto		sockfd (void) const	{ return _sockfd; }
	auto&		sockname (void) const	{ return _sockname; }
	void		wait_read (void) const	{ PTimer::wait_read (sockfd()); }
    private:
	fd_t		_sockfd;
	string		_sockname;
    };
    //}}}
private:
    vector<PExtern>	_isock;
    vector<PListener>	_esock;
private:
    static const iid_t*	s_imports;
    static const iid_t*	s_exports;
};

//{{{ CWICLO_APP -------------------------------------------------------

//{{{2 Imports list

#define GENERATE_PROXY_INTERFACE_COUNTER(arg,proxy)	arg proxy::n_interfaces()
#define GENERATE_PROXY_GET_INTERFACES(arg,proxy)	arg = proxy::get_interfaces(arg);

#define GENERATE_IMPORTS_LIST(proxies)	\
namespace {						\
    constexpr auto generate_imports_list (void) {	\
	constexpr auto ni = 1 SEQ_FOR_EACH(proxies,+,GENERATE_PROXY_INTERFACE_COUNTER);\
	struct { iid_t ia [ni]; } r = {};		\
	auto i = begin(r.ia);				\
	SEQ_FOR_EACH(proxies,i,GENERATE_PROXY_GET_INTERFACES)\
	*i = nullptr;					\
	return r;					\
    }							\
    static constexpr auto s_imports_list = generate_imports_list();\
}							\
const iid_t* App::s_imports = s_imports_list.ia;

//}}}2
//{{{2 Exports list

#define GENERATE_EXPORTS_LIST(proxies)	\
namespace {						\
    constexpr auto generate_exports_list (void) {	\
	constexpr auto ni = 1 SEQ_FOR_EACH(proxies,+,GENERATE_PROXY_INTERFACE_COUNTER);\
	struct { iid_t ia [ni]; } r = {};		\
	auto i = begin(r.ia);				\
	SEQ_FOR_EACH(proxies,i,GENERATE_PROXY_GET_INTERFACES)\
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
    GENERATE_IMPORTS_LIST((PCOM) imports)	\
    GENERATE_EXPORTS_LIST(exports)

//}}}-------------------------------------------------------------------

} // namespace cwiclo
