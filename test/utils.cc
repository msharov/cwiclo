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

template <typename T>
void TestApp::test_bswap (T v) // static
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

int TestApp::run (void)
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
    up1 = make_unique<int>(24);
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
    printf ("clamp(3,4,6) = %d\n", clamp(3,4,6));
    printf ("clamp(6u,5u,6u) = %d\n", clamp(6u,5u,6u));
    printf ("clamp(-3,-10,-6) = %d\n", clamp(-3,-10,-6));
    printf ("clamp(-3l,-6l,6l) = %ld\n", clamp(-3l,-6l,6l));

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
    return EXIT_SUCCESS;
}
