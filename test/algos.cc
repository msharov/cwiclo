// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "../app.h"
using namespace cwiclo;

class TestApp : public App {
    inline		TestApp (void) : App() {}
public:
    static auto&	instance (void) { static TestApp s_app; return s_app; }
    template <typename T>
    static void		test_bswap (T v);
    inline int		run (void);
};

BEGIN_CWICLO_APP (TestApp)
END_CWICLO_APP

//----------------------------------------------------------------------

static void print_int (int i)
{
    printf ("%d ", i);
}

static void print_vector (const vector<int>& v)
{
    printf ("{ ");
    foreach (i, v)
	print_int (*i);
    printf ("}\n");
}

template <typename T>
static void test_big_fill (const size_t size, const T magic)
{
    vector<T> vbig (size);
    fill (vbig.begin() + 1, vbig.end(), magic);		// offset to test prealignment loop
    auto iMismatch = vbig.begin() + 1;
    for (; iMismatch < vbig.end(); ++iMismatch)
	if (*iMismatch != magic)
	    break;
    if (iMismatch == vbig.end())
	printf ("works\n");
    else
	printf ("does not work: mismatch at %zd, =0x%lX\n", distance (vbig.begin(), iMismatch), (unsigned long)(*iMismatch));
}

template <typename T>
static void test_big_copy (const size_t size, const T magic)
{
    vector<T> vbig1 (size), vbig2 (size);
    fill (vbig1, magic);
    copy (vbig1.begin() + 1, vbig1.end(), vbig2.begin() + 1);	// offset to test prealignment loop
    auto im = 1u;
    for (; im < vbig1.size(); ++im)
	if (vbig1[im] != vbig2[im])
	    break;
    if (im == vbig1.size())
	printf ("works\n");
    else
	printf ("does not work: mismatch at %u, 0x%lX != 0x%lX\n", im, (unsigned long)(vbig1[im]), (unsigned long)(vbig2[im]));
}

static const char* bool_name (bool v)
{
    static const char names[][8] = {"false","true"};
    return names[!!v];
}

//----------------------------------------------------------------------

int TestApp::run (void)
{
    static const int c_TestNumbers[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18 };
    auto f = begin(c_TestNumbers);
    auto l = end(c_TestNumbers);
    vector<int> v, buf;
    v.assign (f, l);
    print_vector (v);

    printf ("swap(1,2)\n");
    swap (v[0], v[1]);
    print_vector (v);
    v.assign (f, l);

    printf ("copy(0,8,9)\n");
    copy (v.begin(), v.begin() + 8, v.begin() + 9);
    print_vector (v);
    v.assign (f, l);

    printf ("copy_n(0,8,9)\n");
    copy_n (v.begin(), 8, v.begin() + 9);
    print_vector (v);
    v.assign (f, l);

    printf ("find(10)\n");
    printf ("10 found at offset %zd\n", distance (v.begin(), find (v, 10)));
    printf ("find_if(!(v%%5))\n");
    printf ("5 found at offset %zd\n", distance (v.begin(), find_if (v, [](int i){ return !(i%5); })));
    printf ("find_if_not(v<10)\n");
    printf ("10 found at offset %zd\n", distance (v.begin(), find_if_not (v, [](int i){ return i < 10; })));

    printf ("count(13)\n");
    printf ("%u values of 13, %u values of 18\n", count(v,13), count(v,18));

    printf ("replace(13,666)\n");
    replace (v, 13, 666);
    print_vector (v);
    v.assign (f, l);

    printf ("fill(13)\n");
    fill (v, 13);
    print_vector (v);
    v.assign (f, l);

    printf ("fill_n(5, 13)\n");
    fill_n (v.begin(), 5, 13);
    print_vector (v);
    v.assign (f, l);

    printf ("fill 64083 uint8_t(0x41) ");
    test_big_fill<uint8_t> (64083, 0x41);
    printf ("fill 64083 uint16_t(0x4142) ");
    test_big_fill<uint16_t> (64083, 0x4142);
    printf ("fill 64083 uint32_t(0x41424344) ");
    test_big_fill<uint32_t> (64083, 0x41424344);
    printf ("fill 64083 float(0.4242) ");
    test_big_fill<float> (64083, 0x4242f);
    printf ("fill 64083 uint64_t(0x4142434445464748) ");
    test_big_fill<uint64_t> (64083, UINT64_C(0x4142434445464748));

    printf ("copy 64083 uint8_t(0x41) ");
    test_big_copy<uint8_t> (64083, 0x41);
    printf ("copy 64083 uint16_t(0x4142) ");
    test_big_copy<uint16_t> (64083, 0x4142);
    printf ("copy 64083 uint32_t(0x41424344) ");
    test_big_copy<uint32_t> (64083, 0x41424344);
    printf ("copy 64083 float(0.4242) ");
    test_big_copy<float> (64083, 0.4242f);
    printf ("copy 64083 uint64_t(0x4142434445464748) ");
    test_big_copy<uint64_t> (64083, UINT64_C(0x4142434445464748));

    printf ("merge with (3,5,10,11,11,14)\n");
    const int c_MergeWith[] = { 3,5,10,11,11,14 };
    vector<int> vmerged (v.size() + size(c_MergeWith));
    merge (v.begin(), v.end(), ARRAY_RANGE(c_MergeWith), vmerged.begin());
    print_vector (vmerged);
    v.assign (f, l);

    printf ("inplace_merge with (3,5,10,11,11,14)\n");
    v.insert (v.end(), ARRAY_RANGE(c_MergeWith));
    inplace_merge (v.begin(), v.end() - size(c_MergeWith), v.end());
    print_vector (v);
    v.assign (f, l);

    printf ("remove(13)\n");
    remove (v, 13);
    print_vector (v);
    v.assign (f, l);

    printf ("unique\n");
    unique (v);
    print_vector (v);
    v.assign (f, l);

    printf ("reverse\n");
    reverse (v);
    print_vector (v);
    v.assign (f, l);

    printf ("lower_bound(10)\n");
    print_vector (v);
    printf ("10 begins at position %zd\n", distance (v.begin(), lower_bound (v, 10)));
    v.assign (f, l);

    printf ("upper_bound(10)\n");
    print_vector (v);
    printf ("10 ends at position %zd\n", distance (v.begin(), upper_bound (v, 10)));
    v.assign (f, l);

    printf ("sort\n");
    reverse (v);
    print_vector (v);
    random_shuffle (v);
    sort (v);
    print_vector (v);
    v.assign (f, l);

    printf ("stable_sort\n");
    reverse (v);
    print_vector (v);
    random_shuffle (v);
    stable_sort (v);
    print_vector (v);
    v.assign (f, l);

    printf ("is_sorted\n");
    random_shuffle (v);
    const bool bNotSorted = is_sorted (v.begin(), v.end());
    sort (v);
    const bool bSorted = is_sorted (v.begin(), v.end());
    printf ("unsorted=%s, sorted=%s\n", bool_name(bNotSorted), bool_name(bSorted));
    v.assign (f, l);

    printf ("find_first_of\n");
    static const int c_FFO[] = { 10000, -34, 14, 27 };
    printf ("found 14 at position %zd\n", distance (v.begin(), find_first_of (v.begin(), v.end(), ARRAY_RANGE(c_FFO))));
    v.assign (f, l);

    printf ("max element is %d\n", *max_element (v.begin(), v.end()));
    v.assign (f, l);
    printf ("min element is %d\n", *min_element (v.begin(), v.end()));
    v.assign (f, l);

    printf ("reverse_copy\n");
    buf.resize (v.size());
    reverse_copy (v.begin(), v.end(), buf.begin());
    print_vector (buf);
    v.assign (f, l);

    printf ("rotate_copy\n");
    buf.resize (v.size());
    rotate_copy (v.begin(), v.iat (v.size() / 3), v.end(), buf.begin());
    print_vector (buf);
    v.assign (f, l);

    iota (v.begin(), v.end(), 1);
    printf ("equal(0,9,0) = %u\n", equal (f, f+9, v.cbegin()));
    printf ("accumulate(0,18,3) = %u\n", accumulate (v.begin(), v.end(), 3));
    return EXIT_SUCCESS;
}
