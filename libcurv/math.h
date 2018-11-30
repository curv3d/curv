// Copyright 2017-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_MATH_H
#define LIBCURV_MATH_H

#include <libcurv/value.h>

namespace curv {

struct At_Syntax;

bool isnum(Value a);
bool isbool(Value a);

// Generalized dot product that includes vector dot product and matrix product.
// Same as Mathematica Dot[A,B]. Like APL A+.×B, Python numpy.dot(A,B)
Value dot(Value a, Value b, const At_Syntax& cx);

Value add(Value a, Value b, const At_Syntax& cx);
Value multiply(Value a, Value b, const At_Syntax& cx);

} // namespace curv
#endif // header guard
