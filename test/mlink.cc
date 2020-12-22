// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "../app.h"
#include <ctype.h>
using namespace cwiclo;

class TestApp : public App {
    inline		TestApp (void) : App() {}
public:
    static auto&	instance (void) { static TestApp s_app; return s_app; }
    static void		write_memlink (const memlink& l);
    inline int		run (void);
};

CWICLO_APP_L (TestApp,)

void TestApp::write_memlink (const memlink& l) // static
{
    printf ("memlink{%u}: ", l.size());
    for (auto c : l)
	putchar (isprint(c) ? c : '.');
    putchar ('\n');
}

int TestApp::run (void)
{
    char str[] = "abcdefghijklmnopqrstuvwzyz";
    memlink::const_pointer cstr = str;

    memlink a (ARRAY_BLOCK(str));
    if (a.begin() != str)
	printf ("begin() failed on memlink\n");
    a.static_link (str);
    if (a.begin() + 5 != &str[5])
	printf ("begin() + 5 failed on memlink\n");
    if (!equal (str, a.begin()))
	printf ("memlinks are not equal\n");
    write_memlink (a);
    memlink cb;
    cb.link (ARRAY_BLOCK(str));
    if (cb.data() != cstr)
	printf ("begin() of const failed on memlink\n");
    if (cb.begin() != cstr)
	printf ("begin() failed on memlink\n");
    write_memlink (cb);
    if (a != cb)
	printf ("operator== failed on memlink\n");
    memlink b (ARRAY_BLOCK(str));
    b.resize (size(str)-2);
    a = b;
    if (a.data() != b.data())
	printf ("begin() after assignment failed on memlink\n");
    a.link (ARRAY_BLOCK(str)-1);
    write_memlink (a);
    a.insert (a.begin() + 5, 9);
    fill_n (a.begin() + 5, 9, '-');
    write_memlink (a);
    a.erase (a.begin() + 9, 7);
    fill_n (a.end() - 7, 7, '=');
    write_memlink (a);
    return EXIT_SUCCESS;
}
