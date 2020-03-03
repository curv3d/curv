// Copyright 2017-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_MATH_H
#define LIBCURV_MATH_H

#include <libcurv/value.h>

namespace curv {

struct At_Syntax;

bool issymbol(Value a);

// abstract list API (works for List and Reactive_Value)
bool is_list(Value a);
size_t list_count(Value);
Value list_elem(Value, size_t, const At_Syntax&);

// Generalized dot product that includes vector dot product and matrix product.
// Same as Mathematica Dot[A,B]. Like APL A+.×B, Python numpy.dot(A,B)
Value dot(Value a, Value b, const At_Syntax& cx);

} // namespace curv
#endif // header guard
