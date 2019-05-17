// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "algo.h"
#include <sys/stat.h>

//----------------------------------------------------------------------
namespace cwiclo {
#ifndef UC_VERSION

const char* executable_in_path (const char* efn, char* exe, size_t exesz) noexcept
{
    if (efn[0] == '/' || (efn[0] == '.' && (efn[1] == '/' || efn[1] == '.'))) {
	if (0 != access (efn, X_OK))
	    return nullptr;
	return efn;
    }

    const char* penv = getenv("PATH");
    if (!penv)
	penv = "/bin:/usr/bin:.";
    char path [PATH_MAX];
    if (size(path) < size_t(snprintf (ARRAY_BLOCK(path), "%s/%s"+strlen("%s/"), penv)))
	return nullptr;

    for (char *pf = path, *pl = pf; *pf; pf = pl) {
	while (*pl && *pl != ':')
	    ++pl;
	*pl++ = 0;
	if (exesz < size_t(snprintf (exe, exesz, "%s/%s", pf, efn)))
	    return nullptr;
	if (0 == access (exe, X_OK))
	    return exe;
    }
    return nullptr;
}

unsigned sd_listen_fds (void) noexcept
{
    const char* e = getenv("LISTEN_PID");
    if (!e || getpid() != pid_t(atoi(e)))
	return 0;
    e = getenv("LISTEN_FDS");
    return e ? atoi(e) : 0;
}

int sd_listen_fd_by_name (const char* name) noexcept
{
    const char* na = getenv("LISTEN_FDNAMES");
    if (!na)
	return -1;
    auto namelen = strlen(name);
    for (auto fdi = 0u; *na; ++fdi) {
	const char *ee = strchr(na,':');
	if (!ee)
	    ee = na+strlen(na);
	if (equal_n (name, namelen, na, ee-na))
	    return fdi < sd_listen_fds() ? SD_LISTEN_FDS_START+fdi : -1;
	if (!*ee)
	    break;
	na = ee+1;
    }
    return -1;
}

int mkpath (const char* path, mode_t mode) noexcept
{
    char pbuf [PATH_MAX];
    auto n = size_t(snprintf (ARRAY_BLOCK(pbuf), "%s", path));
    if (n > size(pbuf))
	return -1;
    for (auto f = begin(pbuf), l = f+n; f < l; ++f) {
	f = find (f, l, '/');
	*f = 0;
	if (0 > mkdir (pbuf, mode) && errno != EEXIST)
	    return -1;
	*f = '/';
    }
    return 0;
}

int rmpath (const char* path) noexcept
{
    char pbuf [PATH_MAX];
    if (size(pbuf) < size_t(snprintf (ARRAY_BLOCK(pbuf), "%s", path)))
	return -1;
    while (pbuf[0]) {
	if (0 > rmdir (pbuf))
	    return (errno == ENOTEMPTY || errno == EACCES) ? 0 : -1;
	auto pslash = strrchr (pbuf, '/');
	if (!pslash)
	    pslash = pbuf;
	*pslash = 0;
    }
    return 0;
}
#endif // UC_VERSION

} // namespace cwiclo
