// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_BUILTIN_H
#define CURV_BUILTIN_H

#include <memory>
#include <curv/atom.h>
#include <curv/function.h>
#include <curv/value.h>

namespace curv {

class Meaning;
class Identifier;

struct Builtin : public aux::Shared_Base
{
    virtual Shared<Meaning> to_meaning(const Identifier&) const = 0;
};

struct Builtin_Value : public Builtin
{
    Value value_;
    Builtin_Value(Value v) : value_(std::move(v)) {}
    Builtin_Value(unsigned nargs, Value (*fun)(Frame&))
    :
        value_(make_ref_value<Function>(fun, nargs))
    {}
    virtual Shared<Meaning> to_meaning(const Identifier&) const override;
};

using Namespace = Atom_Map<Shared<const Builtin>>;

extern const Namespace builtin_namespace;

} // namespace curv
#endif // header guard
