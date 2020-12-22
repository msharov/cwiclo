// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "../app.h"
using namespace cwiclo;

class TestApp : public App {
    struct A {
	A (void) { puts ("A::A"); }
	~A (void) { puts ("A::~A"); }
    };
    inline		TestApp (void) : App() {}
public:
    static auto&	instance (void) { static TestApp s_app; return s_app; }
    static void		print_vector (const vector<int>& v);
    static vector<int>	make_iota_vector (unsigned n);
    static vector<int>	subtract_vector (const vector<int>& v1, const vector<int>& v2);
    inline int		run (void);
};

CWICLO_APP_L (TestApp,)

void TestApp::print_vector (const vector<int>& v) // static
{
    putchar ('{');
    for (auto i = 0u; i < v.size(); ++i) {
	if (i)
	    putchar (',');
	printf ("%d", v[i]);
    }
    puts ("}");
}

vector<int> TestApp::make_iota_vector (unsigned n) // static
    { vector<int> r (n); iota (r, 0); return r; }

vector<int> TestApp::subtract_vector (const vector<int>& v1, const vector<int>& v2) // static
{
    vector<int> r (v1.begin(), v1.iat(min(v1.size(),v2.size())));
    for (auto i = 0u; i < r.size(); ++i)
	r[i] = v1[i] - v2[i];
    return r;
}

int TestApp::run (void)
{
    const vector<int> vstd { 8,3,1,2,5,6,1,3,4,9 };
    print_vector (vstd);
    auto v = vstd;
    v.resize (17);
    fill_n (v.iat(vstd.size()), v.size()-vstd.size(), 7);
    v.resize (14);
    print_vector (v);
    auto dv = subtract_vector (v, make_iota_vector (v.size()));
    print_vector (dv);
    v.shrink_to_fit();
    printf ("v: front %d, back %d, [4] %d, capacity %u\n", v.front(), v.back(), v[4], v.capacity());
    v.insert (v.iat(4), {23,24,25});
    v.emplace (v.iat(2), 77);
    v.emplace_back (62);
    v.emplace_back (62);
    v.push_back (62);
    v.erase (v.end()-2);
    v.pop_back();
    print_vector (v);
    random_shuffle (v);
    sort (v);
    print_vector (v);
    printf ("lower_bound(7): %tu\n", lower_bound (v,7)-v.begin());
    printf ("upper_bound(7): %tu\n", upper_bound (v,7)-v.begin());
    printf ("binary_search(3): %tu\n", binary_search (v,3)-v.begin());
    auto s42 = binary_search (v, 42);
    if (s42)
	printf ("binary_search(42): %tu\n", s42-v.begin());
    printf ("count(1): %u\n", count(v,1));
    printf ("count(11): %u\n", count(v,11));
    printf ("count_if(odd): %u\n", count_if(v,[](auto e){return e%2;}));
    iota (v, 2);
    printf ("iota(2): ");
    print_vector (v);
    v.resize (8);
    printf ("generate(pow2()): ");
    unsigned p2i = 0;
    generate (v, [&]{ return 1u << p2i++; });
    print_vector (v);
    printf ("generate_n(pow2(),3): ");
    generate_n (v.begin(), 3, [&]{ return 1u << p2i++; });
    print_vector (v);

    puts ("Constructing vector<A>(3)");
    vector<A> av (3);
    puts ("resize vector<A> to 4");
    av.resize (4);
    puts ("erase 2");
    av.erase (av.iat(2), 2);
    puts ("deallocating");
    return EXIT_SUCCESS;
}
