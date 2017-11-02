// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/thunk.h>
#include <curv/exception.h>
#include <curv/context.h>
#include <curv/function.h>

namespace curv
{

void
Thunk::print(std::ostream& out) const
{
    out << "<thunk>";
}

void
force(Module& nonlocal, slot_t i, Frame& f)
{
    Value& slot = nonlocal.at(i);
    if (slot.is_ref()) {
        auto& ref = slot.get_ref_unsafe();
        switch (ref.type_) {
        case Ref_Value::ty_thunk:
          {
            auto thunk = static_cast<Thunk*>(&ref);
            std::unique_ptr<Frame> f2 {Frame::make(
                thunk->nslots_, f.system, &f, nullptr, &nonlocal)};
            // If there is recursion, force_ref will be called on `slot` during
            // evaluation, which will read and update slot and throw an error.
            slot = thunk->expr_->eval(*f2);
            break;
          }
        case Ref_Value::ty_missing:
            assert(0);
        default:
            break;
        }
    }
}

} // namespace curv
