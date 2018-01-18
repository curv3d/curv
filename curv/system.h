// Copyright Doug Moen 2016-2018.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SYSTEM_H
#define CURV_SYSTEM_H

#include <ostream>
#include <curv/builtin.h>

namespace curv {

/// An abstract interface to the client and operating system.
///
/// The System object is owned by the client, who is responsible for ensuring
/// that it exists for as long as references to it might exist in Curv
/// data structures.
struct System
{
    /// This is the set of standard or builtin bindings
    /// used by the `file` primitive to interpret Curv source files.
    virtual const Namespace& std_namespace() = 0;
    virtual std::ostream& console() = 0;
};

/// Default implementation of the System interface.
struct System_Impl : public System
{
    Namespace std_namespace_;
    std::ostream& console_;
    System_Impl(std::ostream&);
    void load_library(Shared<const String> path);
    virtual const Namespace& std_namespace() override;
    virtual std::ostream& console() override;
};

} // namespace curv
#endif // header guard
