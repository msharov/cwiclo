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
    static void		write_memblock (const memblock& l);
    inline int		run (void);
};

BEGIN_CWICLO_APP (TestApp)
END_CWICLO_APP

void TestApp::write_memblock (const memblock& l) // static
{
    printf ("memblock{%u}: ", l.size());
    for (auto c : l)
	putchar (isprint(c) ? c : '.');
    putchar ('\n');
}

int TestApp::run (void)
{
    char strTest[] = "abcdefghijklmnopqrstuvwxyz";
    const auto strTestLen = __builtin_strlen (strTest);
    const auto cstrTest = strTest;

    memblock a, b;
    a.link (strTest, strTestLen);
    if (a.begin() != strTest)
	printf ("begin() failed on memblock\n");
    if (a.begin() + 5 != &strTest[5])
	printf ("begin() + 5 failed on memblock\n");
    if (!equal (a, strTest))
	printf ("compare failed on memblock\n");
    write_memblock (a);
    b.link (cstrTest, strTestLen);
    if (b.data() != cstrTest)
	printf ("begin() of const failed on memblock\n");
    if (as_const(b).begin() != cstrTest)
	printf ("begin() failed on memblock\n");
    write_memblock (b);
    if (!(a == b))
	printf ("operator== failed on memblock\n");
    b.copy_link();
    if (b.data() == nullptr || b.data() == cstrTest)
	printf ("copy_link failed on memblock\n");
    if (!(a == b))
	printf ("copy_link didn't copy\n");
    b.resize (strTestLen - 2);
    a = b;
    if (a.begin() == b.begin())
	printf ("Assignment does not copy a link\n");
    a.deallocate();
    a.assign (strTest, strTestLen);
    write_memblock (a);
    a.insert (a.begin() + 5, 9);
    fill_n (a.begin() + 5, 9, '-');
    write_memblock (a);
    a.erase (a.begin() + 2, 7);
    fill_n (a.end() - 7, 7, '=');
    write_memblock (a);
    a.resize (0);
    write_memblock (a);
    a.resize (strTestLen + strTestLen / 2);
    fill_n (a.iat(strTestLen), strTestLen/2, '+');
    write_memblock (a);
    return EXIT_SUCCESS;
}
