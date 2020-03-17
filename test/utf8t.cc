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
    static void		widen_utf8 (const string& str, vector<char32_t>& result);
    static void		dump_wchar (char32_t c);
    static void		dump_wchars (const vector<char32_t>& v);
    inline int		run (void);
};

BEGIN_CWICLO_APP (TestApp)
END_CWICLO_APP

void TestApp::widen_utf8 (const string& str, vector<char32_t>& result) // static
{
    result.resize (str.length());
    fill (result, 0);
    copy (str.wbegin(), str.wend(), result.begin());
}

void TestApp::dump_wchar (char32_t c) // static
{
    if (c < CHAR_MAX && isprint(char(c)))
	printf (" '%c'", char(c));
    else
	printf (" %u", unsigned(c));
}

void TestApp::dump_wchars (const vector<char32_t>& v) // static
{
    for (auto i : v)
	dump_wchar (i);
}

int TestApp::run (void)
{
    printf ("Generating Unicode characters ");
    vector<char32_t> wch;
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
    vector<char32_t> dwch;
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

    ws.insert (ws.find('1'), char32_t(1234));
    static constexpr const char32_t c_wchars[] = { 3456, 4567 };
    auto i3 = ws.find('3');
    for (auto i = 0u; i < 2; ++i)
	for (auto j : c_wchars)
	    i3 = ws.insert (i3, j);
    ws.insert (ws.find('3'), char32_t(2345));
    ws.append (char32_t(5678));

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

    return EXIT_SUCCESS;
}
