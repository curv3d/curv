// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/thunk.h>
#include <curv/exception.h>
#include <curv/context.h>

namespace curv
{

void
Thunk::print(std::ostream& out) const
{
    out << "<thunk>";
}

Value
force_ref(Value& slot, const Phrase& identifier, Frame& f)
{
    if (slot.is_ref()) {
        auto& ref {slot.get_ref_unsafe()};
        switch (ref.type_) {
        case Ref_Value::ty_thunk:
          {
            auto expr = static_cast<Thunk*>(&ref)->expr_;
            slot = missing;
            // If there is recursion (eg, `x=x+1`), force_ref will be called
            // recursively. The second call will see the 'missing' value
            // and take the 'ty_missing' branch of the switch.
            slot = expr->eval(f);
            return slot;
          }
        case Ref_Value::ty_missing:
            throw Exception(At_Phrase(identifier, &f),
                "illegal recursive reference");
        default:
            return slot;
        }
    } else {
        return slot;
    }
}

void
force(Value& slot, Frame& f)
{
    if (slot.is_ref()) {
        auto& ref {slot.get_ref_unsafe()};
        switch (ref.type_) {
        case Ref_Value::ty_thunk:
          {
            auto expr = static_cast<Thunk*>(&ref)->expr_;
            // If there is recursion, force_ref will be called on `slot` during
            // evaluation, which will read and update slot and throw an error.
        #if 1
            slot = expr->eval(f);
        #else
            auto val = expr_->eval(f);
            slot = val;
        #endif
            break;
          }
        case Ref_Value::ty_missing:
            assert(0);
        default:
            break;
        }
    }
}

} // end namespace
