// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "memblock.h"
#include "stream.h"
#include "sysutil.h"
#include <fcntl.h>
#include <sys/stat.h>

namespace cwiclo {

int cmemlink::write_file (const char* filename) const
{
    int fd = open (filename, O_WRONLY| O_TRUNC| O_CREAT| O_CLOEXEC, S_IRUSR| S_IWUSR);
    if (fd < 0)
	return -1;
    int r = complete_write (fd, data(), size());
    if (0 > close (fd))
	return -1;
    return r;
}

// Write to a temp file, close it, and move it over filename.
// This method ensures that filename will always be valid, with
// incomplete or otherwise failed writes will end up in a temp file.
//
int cmemlink::write_file_atomic (const char* filename) const
{
    char tmpfilename [PATH_MAX];
    snprintf (ARRAY_BLOCK(tmpfilename), "%s.XXXXXX", filename);

    // Create the temp file
    int ofd = mkstemp (tmpfilename);
    if (ofd < 0)
	return -1;

    // Write the data, minding EINTR
    auto bw = complete_write (ofd, data(), size());
    if (0 > close (ofd))
	return -1;

    // If everything went well, can overwrite the old file
    if (bw >= 0 && 0 > rename (tmpfilename, filename))
	return -1;
    return bw;
}

//----------------------------------------------------------------------

inline static auto next_capacity (memblock::size_type cap)
{
    // Allocate to next power of 2 size block
    auto nshift = log2p1 (cap-1);
    auto newcap = memblock::size_type(1)<<nshift;
    constexpr auto maxshift = bits_in_type<memblock::size_type>::value-1;
    if (nshift >= maxshift) {
	assert (nshift <= maxshift && "memblock maximum allocation size exceeded");
	if (nshift > maxshift)
	    abort();
	--newcap;	// clamp to max_size+1
    }
    return newcap;
}

void memblock::reserve (size_type cap)
{
    if ((cap += zero_terminated()) <= capacity())
	return;
    auto newcap = next_capacity (cap);
    auto oldBlock (is_linked() ? nullptr : data());
    auto newBlock = static_cast<pointer>(_realloc (oldBlock, newcap));
    if (!oldBlock && data())
	copy_n (data(), min (newcap, size() + zero_terminated()), newBlock);
    manage (newBlock, size(), newcap);
}

void memblock::deallocate (void)
{
    assert ((is_linked() || data()) && "Internal error: space allocated, but the pointer is nullptr");
    auto d = is_linked() ? nullptr : data();
    unlink();
    free (d);
}

void memblock::shrink_to_fit (void)
{
    assert (!is_linked() && "call copy_link first");
    auto cap = size() + zero_terminated();
    set_capacity (cap);
    link (static_cast<pointer>(_realloc (data(), cap)), size());
}

void memblock::resize (size_type sz)
{
    reserve (sz);
    shrink (sz);
    if (zero_terminated())
	*end() = 0;
}

void memblock::assign (const_pointer p, size_type sz)
{
    resize (sz);
    copy_n (p, sz, data());
}

auto memblock::insert (const_iterator start, size_type n) -> iterator
{
    const auto ip = start - begin();
    assert (ip <= size());
    resize (size() + n);
    return memlink::insert (iat(ip), n);
}

auto memblock::insert (const_iterator ip, const_pointer s, size_type n) -> iterator
{
    auto ipw = insert (ip,n);
    copy_n (s, n, ipw);
    return ipw;
}

/// Shifts the data in the linked block from \p start + \p n to \p start.
auto memblock::erase (const_iterator start, size_type n) -> iterator
{
    const auto ep = start - begin();
    assert (ep + n <= size());
    memlink::erase (start, n);
    resize (size() - n);
    return iat (ep);
}

memblock::iterator memblock::replace (const_iterator ip, size_type ipn, const_pointer s, size_type sn)
{
    auto dsz = difference_type(sn) - ipn;
    auto ipw = (dsz > 0 ? insert (ip, dsz) : erase (ip, -dsz));
    copy_n (s, sn, ipw);
    return ipw;
}

/// Reads the object from stream \p s
void memblock::read (istream& is)
{
    size_type n; is >> n;
    auto nskip = ceilg (n, stream_alignof(n));
    if (is.remaining() < nskip)
	return;	// errors should have been reported by the message validator
    if (zero_terminated() && n)
	--n;
    reserve (nskip);
    shrink (n);
    is.read (data(), nskip);
}

int memblock::read_file (const char* filename)
{
    int fd = open (filename, O_RDONLY);
    if (fd < 0)
	return -1;
    auto autoclose = make_scope_exit ([&]{ close(fd); });
    struct stat st;
    if (0 > fstat (fd, &st))
	return -1;
    resize (st.st_size);
    return complete_read (fd, data(), st.st_size);
}

//----------------------------------------------------------------------

memblaz::~memblaz (void)
{
    wipe();
}

void memblaz::reserve (size_type cap)
{
    if (cap <= capacity())
	return;
    memblaz r;
    r.memblock::reserve (cap);
    r.memblock::assign (*this);
    swap (r);
}

void memblaz::shrink_to_fit (void)
{
    memblaz r;
    r.manage (_realloc (nullptr, size()), size());
    copy (*this, r.data());
    swap (r);
}

void memblaz::deallocate (void)
{
    wipe();
    memblock::deallocate();
}

void memblaz::resize (size_type sz)
{
    reserve (sz);
    shrink (sz);
}

void memblaz::assign (const_pointer p, size_type sz)
{
    wipe();
    memblock::assign (p, sz);
}

auto memblaz::insert (const_iterator start, size_type n) -> iterator
{
    const auto ip = start - begin();
    assert (ip <= size());
    resize (size() + n);
    return memlink::insert (iat(ip), n);
}

auto memblaz::insert (const_iterator ip, const_pointer s, size_type n) -> iterator
{
    auto ipw = insert (ip,n);
    copy_n (s, n, ipw);
    return ipw;
}

memblaz::iterator memblaz::replace (const_iterator ip, size_type ipn, const_pointer s, size_type sn)
{
    auto dsz = difference_type(sn) - ipn;
    auto ipw = (dsz > 0 ? insert (ip, dsz) : erase (ip, -dsz));
    copy_n (s, sn, ipw);
    return ipw;
}

int memblaz::read_file (const char* filename)
{
    wipe();
    return memblock::read_file (filename);
}

} // namespace cwiclo
