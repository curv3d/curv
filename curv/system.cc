// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/eval.h>
#include <curv/system.h>

using namespace curv;

System_Impl::System_Impl(const String* stdlib_path)
{
    std_namespace_ = builtin_namespace;
    if (stdlib_path != 0) {
        Shared<Module> stdlib = eval_file(*stdlib_path, *this);
        for (auto b : *stdlib)
            std_namespace_[b.first] = make_shared<Builtin_Value>(b.second);
    }
}

const Namespace& System_Impl::std_namespace()
{
    return std_namespace_;
}
