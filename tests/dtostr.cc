#include <gtest/gtest.h>
#include <libcurv/format.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cmath>

using namespace std;
using namespace curv;

bool eq(double x, double y)
{
    if (x == x)
        return x == y && signbit(x) == signbit(y);
    else
        return y != y;
}

void
test(const char*file, int line, double n, const char*str, dfmt::style style)
{
    char buf[DTOSTR_BUFSIZE];
    dtostr(n, buf, style);
    if (strcmp(buf,str) != 0) {
        cout << file << ":" << line << ":"
             << " expected " << str << " got " << buf << "\n";
        EXPECT_TRUE(false);
    }
    if (style == dfmt::JSON && isnan(n))
        ; // JSON uses "null" for NaN
    else {
        double n2 = strtod(buf, NULL);
        if (!eq(n, n2)) {
            cout << file << ":" << line << ":"
                 << " at " << str << ", round trip failed\n";
            EXPECT_TRUE(false);
        }
    }
}

#define DTEST(n,s) test(__FILE__,__LINE__,n,s,dfmt::C)
#define DTEST_STYLE(n,str,style) test(__FILE__,__LINE__,n,str,style)

TEST(curv, dtostr)
{
    DTEST(0.,"0");
    DTEST(0.00001, "1e-5");
    DTEST(0.0001, "0.0001");
    DTEST(0.001, "0.001");
    DTEST(0.01, "0.01");
    DTEST(0.1, "0.1");
    DTEST(1.,"1");
    DTEST(10.,"10");
    DTEST(12.,"12");
    DTEST(100.,"100");
    DTEST(123.,"123");
    DTEST(1000.,"1000");
    DTEST(1234.,"1234");
    DTEST(10000.,"10000");
    DTEST(12345.,"12345");
    DTEST(100000.,"100000");
    DTEST(123456.,"123456");
    DTEST(1000000.,"1000000");
    DTEST(1234567.,"1234567");
    DTEST(10000000.,"10000000");
    DTEST(12345678.,"12345678");
    DTEST(9007199254740992.,"9007199254740992"); // 2^53
    DTEST(9007199254740994.,"9.007199254740994e15"); // 2^53+2
    DTEST(123456789012345.,   "123456789012345");
    DTEST(1234567890123456.,  "1234567890123456");
    DTEST(12345678901234567., "1.2345678901234568e16");
    DTEST(123456789012345678.,"1.2345678901234568e17");
    DTEST(123.456, "123.456");
    DTEST(-0.,"-0");
    DTEST(0./0.,"nan");
    DTEST_STYLE(0./0.,"null", dfmt::JSON);
    DTEST_STYLE(0./0.,"NaN", dfmt::XML);
    DTEST(-0./0.,"nan");
    double infinity = 1./0.;
    DTEST(infinity,"inf");
    DTEST_STYLE(infinity,"1e9999", dfmt::JSON);
    DTEST_STYLE(infinity,"INF", dfmt::XML);
    DTEST(-infinity,"-inf");
    double tiny = nextafter(0., 1.);
    double tiny1 = nextafter(tiny, 1.);
    DTEST(tiny, "5e-324");
    DTEST(tiny1, "1e-323");
    double mtiny = nextafter(0., -1.);
    double mtiny1 = nextafter(mtiny, -1.);
    DTEST(mtiny, "-5e-324");
    DTEST(mtiny1, "-1e-323");
    DTEST(3.14159265358979323846264, "3.141592653589793");
    DTEST(sqrt(2), "1.4142135623730951");
    double huge = nextafter(infinity, 0.);
    DTEST(huge, "1.7976931348623157e308");
}
