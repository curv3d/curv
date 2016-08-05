// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <string>
#include <curv/builtin.h>
#include <curv/exception.h>
#include <curv/function.h>
#include <boost/math/constants/constants.hpp>

using namespace curv;
using namespace std;
using namespace boost::math::double_constants;

Value
builtin_sqrt(Value* args)
{
    double r = sqrt(args[0].get_num_or_nan());
    if (r == r)
        return r;
    else
        throw aux::Exception(stringify("sqrt(",args[0],"): domain error"));
}

const Namespace
curv::builtin_namespace = {
    {"pi", pi},
    {"tau", two_pi},
    {"inf", INFINITY},
    {"null", curv::Value()},
    {"false", curv::Value(false)},
    {"true", curv::Value(true)},
    {"sqrt", curv::make_ref_value<curv::Function>(builtin_sqrt, 1)},
};
