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
    inline int		run (void);
};

BEGIN_CWICLO_APP (TestApp)
END_CWICLO_APP

int TestApp::run (void)
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
    vector<string> strs = {"one","two","three"};

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
    ss << ios::talign<decltype(strs)>() << strs;

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
    os << ios::talign<decltype(strs)>() << strs;
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
    strs.clear();

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
    is >> ios::talign<decltype(strs)>() >> strs;
    if (!is.remaining())
	printf ("Correct number of bytes read\n");
    else
	printf ("Incorrect (%u of %u) number of bytes read\n", b.size()-is.remaining(), b.size());

    printf ("Values:\n"
	"char:	0x%02X\n"
	"uchar:	0x%02X\n"
	"bool:	%d\n"
	"int:	0x%08X\n"
	"uint:	0x%08X\n"
	"long:	0x%08lX\n"
	"ulong:	0x%08lX\n"
	"float:	%.8f\n"
	"double:	%.15f\n"
	"short:	0x%04X\n"
	"ushort:	0x%04X\n"
	"shello:	%s\n"
	"hello:	%s\n",
	static_cast<int>(c), static_cast<int>(uc), static_cast<int>(bv),
	i, ui, li, uli, f, d, static_cast<int>(si), static_cast<int>(usi),
	shello.c_str(), rhello);
    if (strs.size() != 3)
	printf ("Error: strs size is %u\n", strs.size());
    else
	printf ("strs:	{\"%s\",\"%s\",\"%s\"}\n", strs[0].c_str(), strs[1].c_str(), strs[2].c_str());

    if (isatty (STDOUT_FILENO)) {
	printf ("\nBinary dump:\n");
	hexdump (b);
    }

    printf ("\n");
    #define PRINT_ALIGNOF(type)\
	printf ("%u	stream_alignof (" #type ")\n", type_stream_traits<type>::alignment)
    PRINT_ALIGNOF (int8_t);
    PRINT_ALIGNOF (int16_t);
    PRINT_ALIGNOF (uint32_t);
    PRINT_ALIGNOF (uint64_t);
    PRINT_ALIGNOF (simd16_t);
    PRINT_ALIGNOF (cmemlink);
    PRINT_ALIGNOF (memlink);
    PRINT_ALIGNOF (memblock);
    PRINT_ALIGNOF (memblaz);
    PRINT_ALIGNOF (string);
    PRINT_ALIGNOF (vector<uint8_t>);
    PRINT_ALIGNOF (vector<uint64_t>);
    PRINT_ALIGNOF (multiset<string>);
    #undef PRINT_ALIGNOF

    return EXIT_SUCCESS;
}
