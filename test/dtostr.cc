#include <gtest/gtest.h>
#include <aux/dtostr.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cmath>

using namespace std;
using namespace aux;

bool eq(double x, double y)
{
    if (x == x)
        return x == y;
    else
        return y != y;
}

void
test(const char*file, int line, double n, const char*str)
{
    char buf[DTOSTR_BUFSIZE];
    aux::dtostr(n, buf);
    if (strcmp(buf,str) != 0) {
        cout << file << ":" << line << ":"
             << " expected " << str << " got " << buf << "\n";
        EXPECT_TRUE(false);
    }
    double n2 = strtod(buf, NULL);
    if (!eq(n, n2)) {
        cout << file << ":" << line << ":"
             << " at " << str << ", round trip failed\n";
        EXPECT_TRUE(false);
    }
}

#define DTEST(n,s) test(__FILE__,__LINE__,n,s)

TEST(aux, dtostr)
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
    DTEST(10000.,"1e4");
    DTEST(12345.,"12345");
    DTEST(100000.,"1e5");
    DTEST(123456.,"123456");
    DTEST(1000000.,"1e6");
    DTEST(1234567.,"1234567");
    DTEST(10000000.,"1e7");
    DTEST(12345678.,"12345678");
    DTEST(123456789012345.,   "123456789012345");
    DTEST(1234567890123456.,  "1234567890123456");
    DTEST(12345678901234567., "12345678901234568");
    DTEST(123456789012345678.,"123456789012345680");
    DTEST(123.456, "123.456");
    DTEST(-0.,"-0");
    DTEST(0./0.,"nan");
    DTEST(-0./0.,"nan");
    double infinity = 1./0.;
    DTEST(infinity,"inf");
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
