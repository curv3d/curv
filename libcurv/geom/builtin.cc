// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/builtin.h>

#include <libcurv/geom/cpp_program.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/gl_compiler.h>
#include <libcurv/record.h>

namespace curv { namespace geom {

// Run a unit test by compiling it to C++, thus testing the GL compiler's
// C++ code generator.
void
run_cpp_test(const Context& cx, Shared<const Function> func)
{
    Cpp_Program cpp{cx.system()};
    cpp.define_function("test", GL_Type::Bool(), GL_Type::Bool(), func, cx);
    cpp.compile();
    auto test = (void(*)(const bool*,bool*))cpp.get_function("test");
    bool arg = true;
    bool result = false;
    test(&arg, &result);
    if (!result)
        throw Exception(cx, "assertion failed in C++");
}

struct GL_Test_Function : public Legacy_Function
{
    GL_Test_Function() : Legacy_Function(1, "gl_test") {}
    Value call(Frame& args) override
    {
        At_Arg cx(*this, args);
        auto rec = args[0].to<Record>(cx);
        Value nil = Value{List::make(0)};
        rec->each_field(cx, [&](Symbol name, Value val)->void {
            At_Field test_cx{name.c_str(), cx};
            auto func = cast_to_function(val, test_cx);
            if (func == nullptr)
                throw Exception(test_cx, stringify(val," is not a function"));
            bool test_result =
                call_func({func}, nil, args.call_phrase_, args)
                .to_bool(test_cx);
            if (!test_result)
                throw Exception(test_cx, "assertion failed in interpreter");
            run_cpp_test(test_cx, func);
        });
        return {true};
    }
};

void add_builtins(System_Impl& sys)
{
    sys.std_namespace_["gl_test"] =
        make<curv::Builtin_Value>(Value{make<GL_Test_Function>()});
}

}} // namespaces
