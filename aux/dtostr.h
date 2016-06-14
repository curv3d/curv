// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef AUX_DTOSTR_H
#define AUX_DTOSTR_H

#include <ostream>

namespace aux {

/// A class for printing floating point numbers accurately.
///
/// The number is printed as the shortest decimal string that,
/// when read using strtod, reconstructs the original number exactly.
///
/// usage: `stream << dfmt(num);`
///
/// The second argument of `dfmt` is a style, defaulting to `C`.
/// This determines which standard the output conforms to.
/// The 3 styles produce identical output, except in the treatment
/// of infinity and nan. All 3 styles are compatible with `strtod`.
/// * `C` -- infinity prints as "inf", nan prints as "nan"
/// * `JSON` -- infinity prints as "1e9999", nan prints as "null"
/// * `XML` -- infinity prints as "INF", nan prints as "NaN"
struct dfmt
{
    enum style { C, JSON, XML };

    double num_;
    style style_;

    dfmt(double num, style s=C) : num_(num), style_(s) {}
};

std::ostream& operator<<(std::ostream& out, dfmt n);

constexpr int DTOSTR_BUFSIZE = 32;

/// Format a double as the shortest decimal string that,
/// when read using strtod, reconstructs the original number exactly.
void dtostr(double, char[DTOSTR_BUFSIZE], dfmt::style = dfmt::C);

} // namespace aux
#endif // header guard
