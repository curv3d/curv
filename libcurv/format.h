// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_FORMAT_H
#define LIBCURV_FORMAT_H

#include <ostream>

namespace curv {

// # of decimal digits in n
unsigned ndigits(unsigned n);

// A class for printing floating point numbers accurately.
//
// The number is printed as the (usually) shortest decimal string that,
// when read using strtod, reconstructs the original number exactly.
//
// We choose the shortest representation, with 3 exceptions:
// 1. Integers of magnitude <= 2^53 are printed in integer notation,
//    with no decimal point. Integers of magnitude > 2^53 are printed
//    in exponential notation. See bug #103.
// 2. Positive numbers < 1 begin with "0.", not ".", for JSON compatibility.
// 3. Positive numbers < 1 are allowed to have up to 3 leading zeros
//    after the decimal point before we switch to exponential.
//    Eg, "0.0001", not "1e-4".
//
// usage: `stream << dfmt(num);`
//
// The second argument of `dfmt` is a style, defaulting to `C`.
// This determines which standard the output conforms to.
// * `C` -- Infinity prints as "inf", nan prints as "nan".
//   Compatible with `strtod`.
// * `JSON` -- infinity prints as "1e9999", nan prints as "null"
// * `XML` -- infinity prints as "INF", nan prints as "NaN"
//   Compatible with `strtod`.
// * `EXPR` -- print as a floating point expression in a C-like language.
//   Infinity prints as 1.0/0.0, nan prints as 0.0/0.0, integers end with ".0".
struct dfmt
{
    enum style { C, JSON, XML, EXPR };

    double num_;
    style style_;

    dfmt(double num, style s=C) : num_(num), style_(s) {}
};

std::ostream& operator<<(std::ostream& out, dfmt n);

constexpr int DTOSTR_BUFSIZE = 32;

// Format a double as the shortest decimal string that,
// when read using strtod, reconstructs the original number exactly.
void dtostr(double, char[DTOSTR_BUFSIZE], dfmt::style = dfmt::C);

} // namespace
#endif // include guard
