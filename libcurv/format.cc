// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/format.h>

#include <libcurv/num.h>
#include <double-conversion/double-conversion.h>
#include <cstring>
#include <cmath>
using namespace double_conversion;
using Converter = DoubleToStringConverter;

namespace curv {

// # of decimal digits in n
unsigned
ndigits(unsigned n)
{
    unsigned count = 1;
    for (;;) {
        if (n < 10)
            return count;
        n = n / 10;
        ++count;
    }
}

struct StyleSpec { const char* inf; const char* nan; } stylespec[4] = {
    { "inf",     "nan" },     // C
    { "1e9999",  "null" },    // JSON
    { "INF",     "NaN" },     // XML
    { "1.0/0.0", "0.0/0.0" }, // EXPR
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
    // 1. Integers of magnitude <= 2^53 are printed in integer notation,
    //    with no decimal point. Integers of magnitude > 2^53 are printed
    //    in exponential notation. See bug #103.
    // 2. Positive numbers < 1 begin with "0.", not ".", for JSON compatibility.
    // 3. Positive numbers < 1 are allowed to have up to 3 leading zeros
    //    after the decimal point before we switch to exponential.
    //    Eg, "0.0001", not "1e-4".

    // parameters for the algorithm, see above
    constexpr int max_leading_zeros = 3;

    // First, try to output in decimal format. If successful, return early.
    // If the constraints weren't satisfied, then fall through
    // and use exponential format.

    if (decimal_point >= decimal_rep_length) {
        // Integer, possibly with trailing zeros; no decimal point.
        if (std::abs(n) <= max_exact_int) {
            memcpy(p, decimal_rep, decimal_rep_length);
            p += decimal_rep_length;
            int nzeros = decimal_point - decimal_rep_length;
            for (int i = 0; i < nzeros; ++i)
                *p++ = '0';
            if (style == dfmt::EXPR) {
                *p++ = '.';
                *p++ = '0';
            }
            *p = '\0';
            return;
        }
    } else if (decimal_point <= 0) {
        // Fraction < 1; prepend 0. and some leading zeros
        int n_leading_zeros = -decimal_point;
        if (n_leading_zeros <= max_leading_zeros) {
            *p++ = '0';
            *p++ = '.';
            int nzeros = -decimal_point;
            for (int i = 0; i < nzeros; ++i)
                *p++ = '0';
            memcpy(p, decimal_rep, decimal_rep_length);
            p += decimal_rep_length;
            *p = '\0';
            return;
        }
    } else {
        // Numeral with digits before and after the '.'
        for (int i = 0; i < decimal_rep_length; ++i) {
            if (i == decimal_point)
                *p++ = '.';
            *p++ = decimal_rep[i];
        }
        *p = '\0';
        return;
    }

    // Decimal format failed; use exponential format.
    for (int i = 0; i < decimal_rep_length; ++i) {
        *p++ = decimal_rep[i];
        if (i == 0 && decimal_rep_length > 1)
            *p++ = '.';
    }
    *p++ = 'e';
    sprintf(p, "%d", decimal_point - 1);
}

// Print a floating point number accurately.
std::ostream&
operator<<(std::ostream& out, dfmt n)
{
    char buf[32];
    dtostr(n.num_, buf, n.style_);
    out << buf;
    return out;
}

} // namespace curv
