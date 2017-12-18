// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/dtostr.h>
#include <double-conversion/double-conversion.h>
#include <cstring>
#include <cmath>
using namespace double_conversion;

using Converter = DoubleToStringConverter;

namespace curv {

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
    // 1. Positive numbers < 1 begin with "0.", not ".", for JSON compatibility.
    // 2. Integers are allowed to have up to 3 trailing zeros before we switch
    //    to exponential. Eg, "1000", not "1e3".
    // 3. Positive numbers < 1 are allowed to have up to 3 leading zeros
    //    after the decimal point before we switch to exponential.
    //    Eg, "0.0001", not "1e-4".

    // parameters for the algorithm, see above
    constexpr int max_trailing_zeros = 3;
    constexpr int max_leading_zeros = 3;

    // First, try to output in decimal format. If successful, return early.
    // If the constraints weren't satisfied, then fall through
    // and use exponential format.

    if (decimal_point >= decimal_rep_length) {
        // Integer with trailing zeros; no decimal point.
        int n_trailing_zeros = decimal_point - decimal_rep_length;
        if (n_trailing_zeros <= max_trailing_zeros) {
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
