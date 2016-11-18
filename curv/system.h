// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SYSTEM_H
#define CURV_SYSTEM_H

#include <curv/builtin.h>

namespace curv {

/// An abstract interface to the client and operating system.
struct System
{
    /// This is the set of standard or builtin bindings
    /// used by the `file` primitive to interpret Curv source files.
    virtual const Namespace& std_namespace() = 0;
};

/// Default implementation of the System interface.
struct System_Impl : public System
{
    Namespace std_namespace_;
    System_Impl(const String* stdlib_path);
    virtual const Namespace& std_namespace() override;
};

} // namespace curv
#endif // header guard
