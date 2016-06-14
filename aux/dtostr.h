// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef AUX_DTOSTR_H
#define AUX_DTOSTR_H

#include <ostream>

namespace aux {

constexpr int DTOSTR_BUFSIZE = 32;

/// Format a double as the shortest decimal string that,
/// when read using strtod, reconstructs the original number exactly.
void dtostr(double, char[DTOSTR_BUFSIZE]);

/// A class for printing floating point numbers accurately.
///
/// The number is printed as the shortest decimal string that,
/// when read using strtod, reconstructs the original number exactly.
///
/// usage: `stream << dfmt(num);`
struct dfmt
{
    double num_;

    dfmt(double num) : num_(num) {}
};

std::ostream& operator<<(std::ostream& out, dfmt n);

} // namespace aux
#endif // header guard
