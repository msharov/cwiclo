// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "../app.h"
#include "../multiset.h"
using namespace cwiclo;

class TestApp : public App {
    inline		TestApp (void) : App() {}
public:
    static auto&	instance (void) { static TestApp s_app; return s_app; }
    static void		print_multiset (const multiset<int>& v);
    inline int		run (void);
};

CWICLO_APP_L (TestApp,)

void TestApp::print_multiset (const multiset<int>& v) // static
{
    putchar ('{');
    for (auto i = 0u; i < v.size(); ++i) {
	if (i)
	    putchar (',');
	printf ("%d", v[i]);
    }
    puts ("}");
}

int TestApp::run (void)
{
    multiset<int> v {1, 8, 9, 2, 3, 1, 1};
    v.insert ({4, 6, 1, 3, 4});
    printf ("multiset:\t");
    print_multiset (v);
    printf ("erase(3):\t");
    v.erase (3);
    print_multiset (v);
    auto f = v.find (7);
    if (f)
	printf ("7 found at %ld\n", f-v.begin());
    f = v.find (6);
    if (f)
	printf ("6 found at %ld\n", f-v.begin());
    printf ("lower_bound(4) at %ld\n", v.lower_bound (4)-v.begin());
    printf ("upper_bound(4) at %ld\n", v.upper_bound (4)-v.begin());
    printf ("lower_bound(5) at %ld\n", v.lower_bound (5)-v.begin());
    v.insert (v.lower_bound(5), 5);
    print_multiset (v);
    return EXIT_SUCCESS;
}
