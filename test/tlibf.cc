// This file is part of the cwiclo project
//
// Copyright (c) 2018 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the ISC License.

#include "../app.h"
#include "../vector.h"
#include "../multiset.h"
#include "../string.h"
#include "../stream.h"
#include <ctype.h>
#include <stdarg.h>
using namespace cwiclo;

//{{{ LibTestApp -------------------------------------------------------

class LibTestApp : public App {
public:
    static auto&	instance (void) { static LibTestApp s_app; return s_app; }
    inline int		run (void);
private:
    inline		LibTestApp (void) : App() {}
    static void		test_utility (void);
    static void		test_memlink (void);
    static void		test_memblock (void);
    static void		test_vector (void);
    static void		test_multiset (void);
    static void		test_string (void);
    static void		test_stringVector (void);
    static void		test_streams (void);
    static void		write_memlink (const memlink& l);
    static void		write_memblock (const memblock& l);
    static void		print_vector (const vector<int>& v);
    static vector<int>	make_iota_vector (unsigned n);
    static vector<int>	subtract_vector (const vector<int>& v1, const vector<int>& v2);
    static void		print_string (const string& str);
};

//}}}-------------------------------------------------------------------
//{{{ test_utility

template <typename T>
static void test_bswap (T v)
{
    auto vsw (bswap(v));
    auto vbe (vsw), vle (v);
    static const char ok[2][4] = { "bad", "ok" };
    printf ("bswap(%lX) = %lX\n", (unsigned long) v, (unsigned long) vsw);
    printf ("le_to_native(%lX) = %s\n", (unsigned long) v, ok[le_to_native(vle) == v]);
    printf ("native_to_le(%lX) = %s\n", (unsigned long) v, ok[native_to_le(v) == vle]);
    printf ("be_to_native(%lX) = %s\n", (unsigned long) v, ok[be_to_native(vbe) == v]);
    printf ("native_to_be(%lX) = %s\n", (unsigned long) v, ok[native_to_be(v) == vbe]);
}

void LibTestApp::test_utility (void) // static
{
    printf ("divide_ceil(13,5) = %d\n", divide_ceil(13,5));
    printf ("divide_ceil(15,5) = %d\n", divide_ceil(15,5));
    printf ("divide_ceil(-12,5) = %d\n\n", divide_ceil(-12,5));

    printf ("floorg(18,8) = %d\n", floorg(18,8));
    printf ("floorg(-26,25) = %d\n\n", floorg(-26,25));

    printf ("ceilg(5,8) = %d\n", ceilg(5,8));
    printf ("ceilg(5,2) = %d\n", ceilg(5,2));
    printf ("ceilg(17,7) = %d\n", ceilg(17,7));
    printf ("ceilg(14,7) = %d\n", ceilg(14,7));
    printf ("ceilg(-26,25) = %d\n\n", ceilg(-26,25));

    printf ("roundg(5,2) = %d\n", roundg(5,2));
    printf ("roundg(-5,2) = %d\n", roundg(-5,2));
    printf ("roundg(14,7) = %d\n", roundg(14,7));
    printf ("roundg(-1,7) = %d\n", roundg(-1,7));
    printf ("roundg(-14,25) = %d\n\n", roundg(-14,25));

    printf ("divide_round(5,2) = %d\n", divide_round(5,2));
    printf ("divide_round(-5,2) = %d\n", divide_round(-5,2));
    printf ("divide_round(14,9) = %d\n", divide_round(14,9));
    printf ("divide_round(-1,7) = %d\n\n", divide_round(-1,7));

    printf ("square(3) = %u\n", square(3));
    printf ("square(-4) = %u\n\n", square(-4));

    printf ("log2p1(1) = %u\n", log2p1(1));
    printf ("log2p1(4) = %u\n", log2p1(4));
    printf ("log2p1(3827) = %u\n", log2p1(3827u));
    printf ("log2p1(0xFFFFFFF0) = %u\n\n", log2p1(0xFFFFFFF0));

    printf ("ceil2(3) = %u\n", ceil2(3));
    printf ("ceil2(4) = %u\n", ceil2(4));
    printf ("ceil2(3827) = %u\n", ceil2(3827));
    printf ("ceil2(0xFFFFFFF0) = %#lx\n\n", ceil2(0xFFFFFFF0l));

    auto cvp = reinterpret_cast<const void*>(0x1234);
    auto vp = reinterpret_cast<void*>(0x4321);
    printf ("cvp = %lX\n", (uintptr_t) cvp);
    printf ("vp = %lX\n", (uintptr_t) vp);

    auto up1 = make_unique<int> (42);
    up1.swap (make_unique<int> (24));
    printf ("unique_ptr(24) = %d\n", *up1);

    const int32_t c_Numbers[] = { 1, 2, 3, 4, 5 };
    printf ("size(c_Numbers[5]) = %zd\n", size(c_Numbers));
    printf ("size({1,2,3,4,5}) = %zd\n", size({1,2,3,4,5}));

    test_bswap (uint16_t (0x1234));
    test_bswap (uint32_t (0x12345678));
    test_bswap (uint64_t (UINT64_C(0x123456789ABCDEF0)));

    printf ("\nabsv(12) = %u\n", absv(12));
    printf ("absv(-12) = %u\n", absv(-12));
    printf ("sign(12u) = %d\n", sign(12u));
    printf ("sign(-12) = %d\n", sign(-12));
    printf ("sign(0) = %d\n", sign(0));
    printf ("min(3,4) = %d\n", min(3,4));
    printf ("min(6u,1u) = %d\n", min(6u,1u));
    printf ("max(-3,-6) = %d\n", max(-3,-6));
    printf ("max(-3l,6l) = %ld\n", max(-3l,6l));

    printf ("\nequal(abcd,abcd) = %u\n", equal("abcd","abcd"));
    printf ("equal(abcd,acbd) = %u\n", equal("abcd","acbd"));
    printf ("equal(,) = %u\n", equal("",""));

    static constexpr const char mzstr[] = "one\0" "two\0" "three\0" "four\0" "five";
    constexpr auto mzstrn = zstr::nstrs(mzstr);
    printf ("mzstr:");
    for (zstr::cii i (mzstr); i; ++i)
	printf (" %s", *i);
    printf ("\nmzstr.nstrs = %u\n", mzstrn);
    printf ("mzstr.nstrs = %u\n", zstr::nstrs(begin(mzstr),size(mzstr)));
    printf ("mzstr[0] = %s\n", zstr::at(0,mzstr));
    printf ("mzstr[2] = %s\n", zstr::at(2,mzstr));
    printf ("mzstr[4] = %s\n", zstr::at(4,mzstr));
    printf ("mzstr[\"one\"] = %u\n", zstr::index("one",mzstr,mzstrn));
    printf ("mzstr[\"four\"] = %u\n", zstr::index("four",mzstr,mzstrn));
    printf ("mzstr[\"five\"] = %u\n", zstr::index("five",mzstr,mzstrn));
    printf ("mzstr[\"eight\"] = %u\n", zstr::index("eight",mzstr,mzstrn));
}
//}}}
//{{{ test_memlink

void LibTestApp::write_memlink (const memlink& l) // static
{
    printf ("memlink{%u}: ", l.size());
    for (auto c : l)
	putchar (isprint(c) ? c : '.');
    putchar ('\n');
}

void LibTestApp::test_memlink (void) // static
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
}
//}}}-------------------------------------------------------------------
//{{{ test_memblock

void LibTestApp::write_memblock (const memblock& l) // static
{
    printf ("memblock{%u}: ", l.size());
    for (auto c : l)
	putchar (isprint(c) ? c : '.');
    putchar ('\n');
}

void LibTestApp::test_memblock (void) // static
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
}

//}}}-------------------------------------------------------------------
//{{{ test_vector

void LibTestApp::print_vector (const vector<int>& v) // static
{
    putchar ('{');
    for (auto i = 0u; i < v.size(); ++i) {
	if (i)
	    putchar (',');
	printf ("%d", v[i]);
    }
    puts ("}");
}

vector<int> LibTestApp::make_iota_vector (unsigned n) // static
    { vector<int> r (n); iota (r, 0); return r; }

vector<int> LibTestApp::subtract_vector (const vector<int>& v1, const vector<int>& v2) // static
{
    vector<int> r (v1.begin(), v1.iat(min(v1.size(),v2.size())));
    for (auto i = 0u; i < r.size(); ++i)
	r[i] = v1[i] - v2[i];
    return r;
}

struct A {
    A (void) { puts ("A::A"); }
    ~A (void) { puts ("A::~A"); }
};

void LibTestApp::test_vector (void) // static
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
}

//}}}-------------------------------------------------------------------
//{{{ test_multiset

void LibTestApp::test_multiset (void) // static
{
    multiset<int> v {1, 8, 9, 2, 3, 1, 1};
    v.insert ({4, 6, 1, 3, 4});
    printf ("multiset:\t");
    print_vector (v);
    printf ("erase(3):\t");
    v.erase (3);
    print_vector (v);
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
    print_vector (v);
}

//}}}-------------------------------------------------------------------
//{{{ test_string

static void my_format (const char* fmt, ...) PRINTFARGS(1,2);
static void my_format (const char* fmt, ...)
{
    string buf;
    va_list args;
    va_start (args, fmt);
    buf.assignv (fmt, args);
    printf ("Custom vararg my_format: %s\n", buf.c_str());
    va_end (args);
}

void LibTestApp::test_string (void) // static
{
    static const char c_TestString1[] = "123456789012345678901234567890";
    static const char c_TestString2[] = "abcdefghijklmnopqrstuvwxyz";
    static constexpr const string_view c_TestString3 ("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    string s1 (c_TestString1);
    string s2 (ARRAY_RANGE (c_TestString2));
    auto s3 (s1);

    puts (s1.c_str());
    puts (s2.c_str());
    puts (s3.c_str());
    s3.reserve (48);
    s3.resize (20);

    auto i = 0u;
    for (; i < s3.size(); ++ i)
	s3.at(i) = s3.at(i);
    for (i = 0; i < s3.size(); ++ i)
	s3[i] = s3[i];
    printf ("%s\ns3.size() = %u, max_size() = ", s3.c_str(), s3.size());
    if (s3.max_size() == numeric_limits<string::size_type>::max()/2-1)
	printf ("MAX/2-1");
    else
	printf ("%u", s3.max_size());
    printf (", capacity() = %u\n", s3.capacity());

    s1.unlink();
    s1 = c_TestString2;
    s1 += c_TestString3;
    s1 += '$';
    puts (s1.c_str());

    s1 = "Hello";
    s2.deallocate();
    s2 = "World";
    s3 = s1 + s2;
    puts (s3.c_str());
    s3 = "Concatenated ";
    s3 += s1.c_str();
    s3 += s2;
    s3 += " string.";
    puts (s3.c_str());

    if (s1 < s2)
	puts ("s1 < s2");
    if (s1 == s1)
	puts ("s1 == s1");
    if (s1[0] != s1[0])
	puts ("s1[0] != s1[0]");

    string_view s4 (s1);
    if (s1 == s4)
	puts ("s1 == s4");

    if (string_view("abcd") == "abcd")
	puts ("abcd == abcd");
    if (string_view("abc") != "abcd")
	puts ("abc != abcd");
    if (string_view("abcc") < "abcd")
	puts ("abcc < abcd");
    if (string_view("abce") > "abcd")
	puts ("abce > abcd");
    if (string_view("aaaa") < "abc")
	puts ("aaaa < abc");
    if (string_view("abce") > "abc")
	puts ("abce > abc");

    s1 = c_TestString1;
    string s5 (s1.begin() + 4, s1.begin() + 4 + 5);
    string s6 (s1.begin() + 4, s1.begin() + 4 + 5);
    if (s5 == s6)
	puts (string::createf("%s == %s", s5.c_str(), s6.c_str()).c_str());
    string tail (s1.iat(7), s1.end());
    printf ("&s1[7] =\t%s\n", tail.c_str());

    printf ("initial:\t%s\n", s1.c_str());
    printf ("erase([5]-9)\t");
    s1.erase (s1.iat(5), s1.find('9')-s1.iat(5));
    printf ("%s\n", s1.c_str());
    printf ("erase(5,5)\t");
    s1.erase (s1.iat(5), 2U);
    s1.erase (s1.iat(5), 3);
    assert (!*s1.end());
    puts (s1.c_str());
    printf ("push_back('x')\t");
    s1.push_back ('x');
    assert (!*s1.end());
    puts (s1.c_str());
    printf ("pop_back()\n");
    s1.pop_back();
    assert (!*s1.end());
    printf ("insert(10,#)\t");
    s1.insert (s1.iat(10), '#');
    assert (!*s1.end());
    puts (s1.c_str());
    printf ("replace(0,5,@)\t");
    s1.replace (s1.begin(), s1.iat(5),'@',2);
    assert (!*s1.end());
    puts (s1.c_str());

    s1 = c_TestString1;
    printf ("8 found at\t%s\n", s1.find('8'));
    printf ("8 found again\t%s\n", s1.find ('8',s1.find('8')+1));
    printf ("9 found at\t%s\n", s1.find ("9"));
    printf ("7 rfound at\t%s\n", s1.rfind ('7'));
    printf ("7 rfound again\t%s\n", s1.rfind('7', s1.rfind('7') - 1));
    printf ("67 rfound at\t%s\n", s1.rfind ("67"));
    if (!s1.rfind("X"))
	puts ("X was not rfound");
    else
	printf ("X rfound at\t%s\n", s1.rfind ("X"));
    auto poundfound = s1.find ("#");
    if (poundfound)
	printf ("# found at\t%s\n", poundfound);
    printf ("[456] found at\t%s\n", s1.find_first_of ("456",s1.iat(2)));

    s2.clear();
    assert (!*s2.end());
    if (s2.empty())
	printf ("s2 is empty [%s], capacity %u bytes\n", s2.c_str(), s2.capacity());

    s2.assignf ("<const] %d, %s, 0x%08X", 42, "[rfile>", 0xDEADBEEF);
    s2.appendf (", 0%o, appended", 012345);
    s2.insertf (s2.iat(31), "; %u, inserted", 12345);
    printf ("<%u bytes of %u> Format '%s'\n", s2.size(), s2.capacity(), s2.c_str());
    my_format ("'<const] %d, %s, 0x%08X'", 42, "[rfile>", 0xDEADBEEF);
}

//}}}-------------------------------------------------------------------
//{{{ test_stringVector

void LibTestApp::print_string (const string& str) // static
{
    puts (str.c_str());
}

void LibTestApp::test_stringVector (void) // static
{
    vector<string> v = { "Hello world!", "Hello again!", "element3", "element4", "element5_long_element5" };

    auto bogusi = linear_search (v, "bogus");
    if (bogusi)
	printf ("bogus found at position %td\n", bogusi - v.begin());

    foreach (i,v) print_string(*i);

    if (v[2] != "element3")
	printf ("operator== failed\n");
    auto el3i = linear_search (v, string_view("element3"));
    if (el3i)
	printf ("%s found at position %td\n", el3i->c_str(), el3i - v.begin());
    bogusi = linear_search (v, "bogus");
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
}
//}}}-------------------------------------------------------------------
//{{{ test_streams

void LibTestApp::test_streams (void) // static
{
    const uint8_t magic_Char = 0x12;
    const uint16_t magic_Short = 0x1234;
    const uint32_t magic_Int = 0x12345678;
    const float magic_Float = 0.12345678;
    const double magic_Double = 0.123456789123456789;
    const bool magic_Bool = true;

    char c = magic_Char;
    unsigned char uc = magic_Char;
    int i = magic_Int;
    short si = magic_Short;
    long li = magic_Int;
    unsigned int ui = magic_Int;
    unsigned short usi = magic_Short;
    unsigned long uli = magic_Int;
    float f = magic_Float;
    double d = magic_Double;
    bool bv = magic_Bool;

    sstream ss;
    ss << c;
    ss << uc;
    ss << ios::talign<bool>() << bv;
    ss << ios::talign<int>() << i;
    ss << ui;
    ss << ios::align(8) << li;
    ss << uli;
    ss << ios::talign<float>() << f;
    ss << ios::talign<double>() << d;
    ss << si;
    ss << usi;
    static constexpr const char hello[] = "Hello world!";
    ss << hello;
    ss.write_strz (hello);

    memblock b;
    b.resize (ss.size());
    fill (b, '\xcd');
    ostream os (b);

    os << c;
    os << uc;
    os << ios::talign<bool>() << bv;
    os << ios::talign<int>() << i;
    os << ui;
    os << ios::align(8) << li;
    os << uli;
    os << ios::talign<float>() << f;
    os << ios::talign<double>() << d;
    os << si;
    os << usi;
    os << hello;
    os.write_strz (hello);
    if (!os.remaining())
	printf ("Correct number of bytes written\n");
    else
	printf ("Incorrect (%u of %u) number of bytes written\n", b.size()-os.remaining(), b.size());

    c = 0;
    uc = 0;
    bv = false;
    i = ui = li = uli = 0;
    f = 0; d = 0;
    si = usi = 0;

    istream is (b);
    is >> c;
    is >> uc;
    is >> ios::talign<bool>() >> bv;
    is >> ios::talign<int>() >> i;
    is >> ui;
    is >> ios::align(8) >> li;
    is >> uli;
    is >> ios::talign<float>() >> f;
    is >> ios::talign<double>() >> d;
    is >> si;
    is >> usi;
    string shello;
    is >> shello;
    auto rhello = is.read_strz();
    if (!is.remaining())
	printf ("Correct number of bytes read\n");
    else
	printf ("Incorrect (%u of %u) number of bytes read\n", b.size()-is.remaining(), b.size());

    printf ("Values:\n"
	"char:    0x%02X\n"
	"u_char:  0x%02X\n"
	"bool:    %d\n"
	"int:     0x%08X\n"
	"u_int:   0x%08X\n"
	"long:    0x%08lX\n"
	"u_long:  0x%08lX\n"
	"float:   %.8f\n"
	"double:  %.15f\n"
	"short:   0x%04X\n"
	"u_short: 0x%04X\n"
	"shello:  %s\n"
	"hello:   %s\n",
	static_cast<int>(c), static_cast<int>(uc), static_cast<int>(bv),
	i, ui, li, uli, f, d, static_cast<int>(si), static_cast<int>(usi),
	shello.c_str(), rhello);

    if (isatty (STDOUT_FILENO)) {
	printf ("\nBinary dump:\n");
	hexdump (b);
    }
}
//}}}-------------------------------------------------------------------
//{{{ UTF8

static void widen_utf8 (const string& str, vector<wchar_t>& result)
{
    result.resize (str.length());
    fill (result, 0);
    copy (str.wbegin(), str.wend(), result.begin());
}

static void dump_wchar (wchar_t c)
{
    if (c < CHAR_MAX && isprint(char(c)))
	printf (" '%c'", char(c));
    else
	printf (" %u", unsigned(c));
}

static void dump_wchars (const vector<wchar_t>& v)
{
    for (auto i : v)
	dump_wchar (i);
}

static void test_utf8 (void)
{
    printf ("Generating Unicode characters ");
    vector<wchar_t> wch;
    wch.resize (0xffff);
    iota (wch, 0);
    printf ("%u - %u\n", unsigned(wch[0]), unsigned(wch.back()));

    puts ("Encoding to utf8.");
    string encoded;
    encoded.reserve (wch.size()*2);
    for (auto c : wch)
	encoded += c;

    for (auto i = 0u, ci = 0u; i < encoded.size(); ++ci) {
	auto seqb = utf8::ibytes(encoded[i]), cntb = utf8::obytes(ci);
	if (seqb != cntb)
	    printf ("Char %x encoded in %u bytes instead of %u\n", ci, seqb, cntb);
	unsigned char h1 = 0xff << (8-seqb), m1 = 0xff << (7-seqb);
	if (ci <= 0x7f) {
	    h1 = 0;
	    m1 = 0x80;
	}
	if ((encoded[i] & m1) != h1)
	    printf ("Char %x has incorrect encoded header %02x instead of %02hhx\n", ci, encoded[i] & m1, h1);
	++i;
	for (unsigned j = 1; j < seqb; ++j, ++i)
	    if ((encoded[i] & 0xc0) != 0x80)
		printf ("Char %x has incorrect intrabyte %u: %02hhx\n", ci, j, encoded[i]);
    }

    puts ("Decoding back.");
    vector<wchar_t> dwch;
    widen_utf8 (encoded, dwch);

    printf ("Comparing.\nsrc = %u chars, encoded = %u chars, decoded = %u\n", wch.size(), encoded.size(), dwch.size());
    auto ndiffs = 0u;
    for (auto i = 0u; i < min (wch.size(), dwch.size()); ++i) {
	if (wch[i] != dwch[i]) {
	    printf ("%u != %u\n", unsigned(wch[i]), unsigned(dwch[i]));
	    ++ndiffs;
	}
    }
    printf ("%u differences between src and decoded.\n", ndiffs);

    puts ("Testing wide character string::insert");
    string ws ("1234567890");

    ws.insert (ws.find('1'), wchar_t(1234));
    static constexpr const wchar_t c_wchars[] = { 3456, 4567 };
    auto i3 = ws.find('3');
    for (auto i = 0u; i < 2; ++i)
	for (auto j : c_wchars)
	    i3 = ws.insert (i3, j);
    ws.insert (ws.find('3'), wchar_t(2345));
    ws.append (wchar_t(5678));

    printf ("Values[%u]:", ws.length());
    for (auto j = ws.wbegin(); j < ws.wend(); ++j)
	dump_wchar (*j);

    printf ("\nCharacter offsets:");
    for (auto k = ws.wbegin(); k < ws.wend(); ++k)
	printf (" %zu", k.base()-ws.begin());

    printf ("\nErasing character %u: ", ws.length()-1);
    ws.erase (ws.wiat(ws.length()-1).base(), ws.end());
    widen_utf8 (ws, dwch);
    dump_wchars (dwch);

    printf ("\nErasing 2 characters after '2': ");
    ws.erase (ws.find('2')+1, utf8::obytes(c_wchars));
    widen_utf8 (ws, dwch);
    dump_wchars (dwch);
    printf ("\n");
}

//}}}-------------------------------------------------------------------
//{{{ run tests

int LibTestApp::run (void)
{
    using stdtestfunc_t	= void (*)(void);
    static const stdtestfunc_t c_Tests[] = {
	test_utility,
	test_memlink,
	test_memblock,
	test_vector,
	test_multiset,
	test_string,
	test_stringVector,
	test_streams,
	test_utf8
    };
    for (auto& i : c_Tests) {
	puts ("######################################################################");
	i();
    }
    return EXIT_SUCCESS;
}

BEGIN_CWICLO_APP (LibTestApp)
END_CWICLO_APP

//}}}-------------------------------------------------------------------
