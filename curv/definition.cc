// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <functional>
#include <iostream>

#include <curv/definition.h>
#include <curv/exception.h>
#include <curv/context.h>

namespace curv
{

void
Data_Definition::add_to_scope(Scope& scope)
{
    unsigned unitnum = scope.begin_unit(share(*this));
    slot_ = scope.add_binding(name_, unitnum);
    scope.end_unit(unitnum, share(*this));
}
void
Function_Definition::add_to_scope(Scope& scope)
{
    unsigned unitnum = scope.begin_unit(share(*this));
    slot_ = scope.add_binding(name_, unitnum);
    scope.end_unit(unitnum, share(*this));
}
void
Data_Definition::analyze(Environ& env)
{
    definiens_expr_ = analyze_op(*definiens_phrase_, env);
}
void
Function_Definition::analyze(Environ& env)
{
    auto expr = analyze_op(*definiens_phrase_, env);
    auto lambda = cast<Lambda_Expr>(expr);
    assert(lambda != nullptr);
    definiens_expr_ = lambda;
}
Shared<Operation>
Data_Definition::make_action(slot_t indirect_slot)
{
    if (indirect_slot != (slot_t)(-1)) {
        return make<Indirect_Assign>(source_, indirect_slot, slot_, definiens_expr_);
    } else {
        return make<Let_Assign>(source_, slot_, definiens_expr_, false);
    }
}
Shared<Operation>
Function_Definition::make_action(slot_t indirect_slot)
{
    if (indirect_slot != (slot_t)(-1)) {
        return make<Indirect_Assign>(source_, indirect_slot, slot_, definiens_expr_);
    } else {
        return make<Let_Assign>(source_, slot_, definiens_expr_, false);
    }
}
void
Compound_Definition_Base::add_to_scope(Scope& scope)
{
    for (auto &e : *this) {
        if (e.definition_ == nullptr)
            scope.add_action(e.phrase_);
        else
            e.definition_->add_to_scope(scope);
    }
}
void
Sequential_Scope::analyze(Abstract_Definition& def)
{
    assert(def.kind_ == Abstract_Definition::k_sequential);
    def.add_to_scope(*this);
    parent_->frame_maxslots_ = frame_maxslots_;
    if (target_is_module_)
        executable_.module_dictionary_ = dictionary_;
}
Shared<Meaning>
Sequential_Scope::single_lookup(const Identifier& id)
{
    auto b = dictionary_->find(id.atom_);
    if (b != dictionary_->end()) {
        if (target_is_module_) {
            return make<Indirect_Strict_Ref>(
                share(id), executable_.module_slot_, b->second);
        } else {
            return make<Let_Ref>(share(id), b->second);
        }
    }
    return nullptr;
}
void
Sequential_Scope::add_action(Shared<const Phrase> phrase)
{
    executable_.actions_.push_back(analyze_action(*phrase, *this));
}
unsigned
Sequential_Scope::begin_unit(Shared<Unitary_Definition> unit)
{
    unit->analyze(*this);
    return 0;
}
slot_t
Sequential_Scope::add_binding(Shared<const Identifier> name, unsigned unitno)
{
    (void)unitno;
    if (dictionary_->find(name->atom_) != dictionary_->end())
        throw Exception(At_Phrase(*name, *parent_),
            stringify(name->atom_, ": multiply defined"));
    slot_t slot = (target_is_module_ ? dictionary_->size() : make_slot());
    (*dictionary_)[name->atom_] = slot;
    return slot;
}
void
Sequential_Scope::end_unit(unsigned unitno, Shared<Unitary_Definition> unit)
{
    (void)unitno;
    executable_.actions_.push_back(
        unit->make_action(executable_.module_slot_));
}

} // namespace curv
