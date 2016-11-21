// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <string>

#include <boost/math/constants/constants.hpp>

#include <curv/arg.h>
#include <curv/builtin.h>
#include <curv/eval.h>
#include <curv/exception.h>
#include <curv/file.h>
#include <curv/function.h>
#include <curv/system.h>

using namespace curv;
using namespace std;
using namespace boost::math::double_constants;

Shared<Meaning>
Builtin_Value::to_meaning(const Identifier& id) const
{
    return make<Constant>(share(id), value_);
}

/*
map["sqrt"] = {1, [](Frame& args) -> Value {
    double r = sqrt(args[0].get_num_or_nan());
    if (r == r)
        return r;
    else
        throw Exception(At_Arg(0, &args),
            stringify("sqrt(",args[0],"): domain error"));
}};
*/

Value
builtin_sqrt(Frame& args)
{
    double r = sqrt(args[0].get_num_or_nan());
    if (r == r)
        return r;
    else
        throw Exception(At_Arg(0, &args),
            stringify("sqrt(",args[0],"): domain error"));
}

Value
builtin_len(Frame& args)
{
    auto& list {arg_to_list(args[0], At_Arg(0, &args))};
    return {double(list.size())};
}

Value
builtin_file(Frame& f)
{
    At_Arg ctx0(0, &f);
    String& path {arg_to_string(f[0], ctx0)};
    auto file = make<File_Script>(share(path), ctx0);
    return {eval_script(*file, f.system, &f)};
}

/// The meaning of a call to `echo`, such as `echo("foo")`.
struct Echo_Action : public Action
{
    std::vector<Shared<Operation>> argv_;
    Echo_Action(
        Shared<const Phrase> source,
        std::vector<Shared<Operation>> argv)
    :
        Action(std::move(source)),
        argv_(std::move(argv))
    {}
    virtual void exec(Frame& f) const override
    {
        std::ostream& out = f.system.console();
        out << "ECHO: ";
        bool first = true;
        for (auto a : argv_) {
            if (!first) out << ",";
            out << a->eval(f);
            first = false;
        }
        out << "\n";
    }
};
/// The meaning of the phrase `echo` in isolation.
struct Echo_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        return make<Echo_Action>(share(ph), ph.analyze_args(env));
    }
};

const Namespace
curv::builtin_namespace =
{
    {"pi", make<Builtin_Value>(pi)},
    {"tau", make<Builtin_Value>(two_pi)},
    {"inf", make<Builtin_Value>(INFINITY)},
    {"null", make<Builtin_Value>(Value())},
    {"false", make<Builtin_Value>(Value(false))},
    {"true", make<Builtin_Value>(Value(true))},
    {"sqrt", make<Builtin_Value>(1, builtin_sqrt)},
    {"len", make<Builtin_Value>(1, builtin_len)},
    {"file", make<Builtin_Value>(1, builtin_file)},
    {"echo", make<Builtin_Meaning<Echo_Metafunction>>()},
};
