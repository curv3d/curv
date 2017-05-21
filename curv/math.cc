// Copyright Doug Moen 2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/math.h>
#include <curv/context.h>
#include <curv/array_op.h>

namespace curv {

struct Context;

Value add(Value a, Value b, const Context& cx)
{
    struct Scalar_Op {
        static double f(double x, double y) { return x + y; }
        static const char* name() { return "+"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x,"+",y);
        }
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    return array_op.op(a, b, cx);
}

Value multiply(Value a, Value b, const Context& cx)
{
    struct Scalar_Op {
        static double f(double x, double y) { return x * y; }
        static const char* name() { return "*"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x,"*",y);
        }
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    return array_op.op(a, b, cx);
}

// Generalized dot product that includes vector dot product and matrix product.
// Same as Mathematica Dot[A,B]. Like APL A+.×B, Python numpy.dot(A,B)
//  dot(a,b) =
//    if (len a > 0 && is_list(a[0]))
//      [for(row=a)dot(row,b)]  // matrix*...
//    else
//      sum(a*b),               // vector*...
Value dot(Value a, Value b, const Context& cx)
{
    auto av = a.to<List>(cx);
    auto bv = b.to<List>(cx);
    if (av->size() > 0 && av->at(0).dycast<List>()) {
        Shared<List> result = List::make(av->size());
        for (size_t i = 0; i < av->size(); ++i) {
            result->at(i) = dot(av->at(i), b, cx);
        }
        return {result};
    } else {
        if (av->size() != bv->size())
            throw Exception(cx, stringify("list of size ",av->size(),
                "can't be multiplied by list of size ",bv->size()));
        Value result = {0.0};
        for (size_t i = 0; i < av->size(); ++i)
            result = add(result, multiply(av->at(i), bv->at(i), cx), cx);
        return result;
    }
}

} // namespace curv
