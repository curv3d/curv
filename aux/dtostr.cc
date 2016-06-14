// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <aux/dtostr.h>
#include <double-conversion/double-conversion.h>
#include <cstring>
#include <cmath>
using namespace double_conversion;

using Converter = DoubleToStringConverter;

namespace aux {

// I don't like how the choice is made between exponential and normal format
#if 0
DoubleToStringConverter cvt(
    0,      // flags
    "inf",  // infinity_symbol
    "nan",  // nan_symbol
    'e',    // exponent_character
    -4,     // decimal_in_shortest_low
            // 0.0001 -> "0.0001"; 0.00001 -> "1e-5"
    4,      // decimal_in_shortest_high
            // 1000 -> "1000", 10000 -> "1e4"
    0,      // max_leading_padding_zeroes_in_precision_mode (unused)
    0       // max_trailing_padding_zeroes_in_precision_mode (unused)
);

void dtostr(double n, char* buf)
{
    StringBuilder builder(buf, DTOSTR_BUFSIZE);
    cvt.ToShortest(n, &builder);
}
#endif

struct StyleSpec { const char* inf; const char* nan; } stylespec[3] = {
    { "inf",    "nan" },   // C
    { "1e9999", "null" },  // JSON
    { "INF",    "NaN" },   // XML
};

void dtostr(double n, char* buf, dfmt::style style)
{
    if (n != n) {
        strcpy(buf, stylespec[style].nan);
        return;
    }
    char* p = buf;
    if (std::signbit(n)) {
        *p++ = '-';
        n = -n;
    }
    if (n == INFINITY) {
        strcpy(p, stylespec[style].inf);
        return;
    }

    int decimal_point;
    bool sign;
    const int kDecimalRepCapacity = Converter::kBase10MaximalLength + 1;
    char decimal_rep[kDecimalRepCapacity];
    int decimal_rep_length;

    Converter::DoubleToAscii(n, Converter::SHORTEST, 0,
        decimal_rep, kDecimalRepCapacity,
        &sign, &decimal_rep_length, &decimal_point);

    // Choose between decimal (eg, 42) and exponential (eg, 4.2e1) formats.

    // We choose the shortest representation, with 3 exceptions:
    // 1. Positive numbers < 1 begin with "0.", not ".", for JSON compatibility.
    // 2. Integers are allowed to have up to 3 trailing zeros before we switch
    //    to exponential. Eg, "1000", not "1e3".
    // 3. Positive numbers < 1 are allowed to have up to 3 leading zeroes
    //    after the decimal point before we switch to exponential.
    //    Eg, "0.0001", not "1e-4".
    // dbonus is the 'decimal bonus', and implements rules 2 and 3 above.
    int dbonus = 0;

    // compute overhead of exponential format
    int exp_overhead = 1;   // for "e"
    if (decimal_rep_length > 1)
        ++exp_overhead;     // for "."
    // value of exponent in exponential format
    int e = decimal_point - 1;
    if (e < 0) {
        ++exp_overhead; // for "-"
        e = -e;
    }
    if (e > 99)
        exp_overhead += 3;
    else if (e > 9)
        exp_overhead += 2;
    else
        exp_overhead += 1;

    // compute overhead of decimal format
    int decimal_overhead = 0;
    if (decimal_point >= decimal_rep_length) {
        // number of '0's to append; no decimal point
        decimal_overhead = decimal_point - decimal_rep_length;
        dbonus = 1;
    } else if (decimal_point <= 0) {
        // prepend 0. and some zeroes
        decimal_overhead = 2 - decimal_point;
        dbonus = 2;
    } else {
        // insert '.' in the middle
        decimal_overhead = 1;
    }

    // Output the numeral in decimal or exponential form
    if (decimal_overhead - dbonus <= exp_overhead) {
        // decimal form
        if (decimal_point <= 0) {
            // prepend 0. and some zeroes
            *p++ = '0';
            *p++ = '.';
            int nzeros = -decimal_point;
            for (int i = 0; i < nzeros; ++i)
                *p++ = '0';
            memcpy(p, decimal_rep, decimal_rep_length);
            p += decimal_rep_length;
        } else if (decimal_point >= decimal_rep_length) {
            // append some zeroes; no decimal point
            memcpy(p, decimal_rep, decimal_rep_length);
            p += decimal_rep_length;
            for (int i = 0; i < decimal_overhead; ++i)
                *p++ = '0';
        } else {
            // insert '.' in the middle
            for (int i = 0; i < decimal_rep_length; ++i) {
                if (i == decimal_point)
                    *p++ = '.';
                *p++ = decimal_rep[i];
            }
        }
        *p = '\0';
    } else {
        // exponential form
        for (int i = 0; i < decimal_rep_length; ++i) {
            *p++ = decimal_rep[i];
            if (i == 0 && decimal_rep_length > 1)
                *p++ = '.';
        }
        *p++ = 'e';
        sprintf(p, "%d", decimal_point - 1);
    }
}

// Print a floating point number accurately.
std::ostream&
operator<<(std::ostream& out, dfmt n)
{
    char buf[32];
    dtostr(n.num_, buf);
    out << buf;
    return out;
}

} // namespace aux
