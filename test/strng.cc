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
    static void		my_format (const char* fmt, ...) PRINTFARGS(1,2);
    inline int		run (void);
};

BEGIN_CWICLO_APP (TestApp)
END_CWICLO_APP

void TestApp::my_format (const char* fmt, ...) // static
{
    string buf;
    va_list args;
    va_start (args, fmt);
    buf.assignv (fmt, args);
    printf ("Custom vararg my_format: %s\n", buf.c_str());
    va_end (args);
}

int TestApp::run (void)
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

    s1.deallocate();
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

    return EXIT_SUCCESS;
}
