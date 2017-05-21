// Copyright Doug Moen 2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_MATH_H
#define CURV_MATH_H

#include <curv/value.h>

namespace curv {

struct Context;

// Generalized dot product that includes vector dot product and matrix product.
// Same as Mathematica Dot[A,B]. Like APL A+.×B, Python numpy.dot(A,B)
Value dot(Value a, Value b, const Context& cx);

Value add(Value a, Value b, const Context& cx);
Value multiply(Value a, Value b, const Context& cx);

} // namespace curv
#endif // header guard
