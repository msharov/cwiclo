// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "../app.h"
#include <stdarg.h>
using namespace cwiclo;

class TestApp : public App {
    inline		TestApp (void) : App() {}
public:
    static auto&	instance (void) { static TestApp s_app; return s_app; }
    static void		print_string (const string& str) { puts (str.c_str()); }
    inline int		run (void);
};

CWICLO_APP_L (TestApp,)

int TestApp::run (void)
{
    vector<string> v = { "Hello world!", "Hello again!", "element3", "element4", "element5_long_element5" };

    auto bogusi = find (v, "bogus");
    if (bogusi)
	printf ("bogus found at position %td\n", bogusi - v.begin());

    foreach (i,v) print_string(*i);

    if (v[2] != "element3")
	printf ("operator== failed\n");
    auto el3i = find (v, string_view("element3"));
    if (el3i)
	printf ("%s found at position %td\n", el3i->c_str(), el3i - v.begin());
    bogusi = find (v, "bogus");
    if (bogusi)
	printf ("%s found at position %td\n", bogusi->c_str(), bogusi - v.begin());

    vector<string> v2;
    v2 = v;
    v = v2;
    v.erase (v.end(), v.end());
    printf ("After erase (end,end):\n");
    foreach (i,v) print_string(*i);
    v = v2;
    v.erase (v.begin() + 2, 2);
    printf ("After erase (2,2):\n");
    foreach (i,v) print_string(*i);
    v = v2;
    v.pop_back();
    printf ("After pop_back():\n");
    foreach (i,v) print_string(*i);
    v = v2;
    v.insert (v.begin() + 1, v2.begin() + 1, v2.begin() + 1 + 3);
    printf ("After insert(1,1,3):\n");
    foreach (i,v) print_string(*i);
    v = v2;
    sort (v);
    printf ("After sort:\n");
    foreach (i,v) print_string(*i);
    el3i = binary_search (v, "element3");
    if (el3i)
	printf ("%s found at position %td\n", el3i->c_str(), el3i - v.begin());
    bogusi = binary_search (v, "bogus");
    if (bogusi)
	printf ("%s found at position %td\n", bogusi->c_str(), bogusi - v.begin());

    return EXIT_SUCCESS;
}
