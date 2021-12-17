// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/io/builtin.h>

#include <libcurv/io/cpp_program.h>

#include <libcurv/analyser.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/meanings.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/record.h>

namespace curv { namespace io {

// Run a unit test by compiling it to C++, thus testing the SC compiler's
// C++ code generator.
void
run_cpp_test(const Context& cx, Source_State& ss, Shared<const Function> func)
{
    Cpp_Program cpp{ss};
    cpp.define_function("test", SC_Type::Bool(), SC_Type::Bool(), func, cx);
    cpp.compile(cx);
    auto test = (void(*)(const bool*,bool*))cpp.get_function("test");
    bool arg = true;
    bool result = false;
    test(&arg, &result);
    if (!result) {
        cpp.preserve_tempfile();
        throw Exception(cx, stringify(
            "assertion failed in C++; see ",cpp.path_));
    }
}
struct SC_Test_Action : public Just_Action
{
    Shared<Operation> arg_;
    SC_Test_Action(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Just_Action(move(syntax)),
        arg_(move(arg))
    {}
    virtual void exec(Frame& f, Executor&) const override
    {
        Value arg = arg_->eval(f);
        At_Phrase cx(*arg_->syntax_, f);
        auto rec = arg.to<Record>(cx);
        Value nil = Value{List::make(0)};
        rec->each_field(cx, [&](Symbol_Ref name, Value val)->void {
            At_Field test_cx{name.c_str(), cx};
            auto func = maybe_function(val, test_cx);
            if (func == nullptr)
                throw Exception(test_cx, stringify(val," is not a function"));
            bool test_result =
                call_func({func}, nil, syntax_, f)
                .to_bool(test_cx);
            if (!test_result)
                throw Exception(test_cx, "assertion failed in interpreter");
            run_cpp_test(test_cx, f.sstate_, func);
        });
    }
};
struct SC_Test_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        return make<SC_Test_Action>(share(ph), analyse_op(*ph.arg_, env));
    }
    virtual void print_help(std::ostream& out) const override
    {
        out <<
            "sc_test {tagname1: testfunction1, tagname2: testfunction2, ...}\n"
            "  Each test function maps an argument (which is ignored) onto a boolean value.\n"
            "  Execute each test function, and if any test function returns false, report an assertion failure,\n"
            "  mentioning the tagname in the error message. Each testfunction is evaluated twice,\n"
            "  once in the interpreter, and once by compiling to native code using the SubCurv compiler.\n"
            "  This is used to write SubCurv unit tests.\n";
    }
};

void add_builtins(System_Impl& sys)
{
    sys.std_namespace_[make_symbol("sc_test")] =
        make<Builtin_Meaning<SC_Test_Metafunction>>();
}

}} // namespaces
