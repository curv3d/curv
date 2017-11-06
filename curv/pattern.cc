// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/pattern.h>
#include <curv/phrase.h>
#include <curv/definition.h>
#include <curv/exception.h>
#include <curv/context.h>

namespace curv {

struct Id_Pattern : public Pattern
{
    slot_t slot_;

    Id_Pattern(slot_t slot) : slot_(slot) {}

    virtual void analyze(Environ&) override
    {
    }
    virtual void exec(
        Value* slots, Value value, const Context&, Frame&) override
    {
        slots[slot_] = value;
    }
};

struct List_Pattern : public Pattern
{
    std::vector<Shared<Pattern>> items_;

    List_Pattern(std::vector<Shared<Pattern>> items)
    :
        items_(std::move(items))
    {}

    virtual void analyze(Environ&) override
    {
    }
    virtual void exec(
        Value* slots, Value val, const Context& valcx, Frame& f) override
    {
        auto list = val.to<List>(valcx);
        list->assert_size(items_.size(), valcx);
        for (size_t i = 0; i < items_.size(); ++i)
            items_[i]->exec(slots, list->at(i), At_Index(i, valcx), f);
    }
};

Shared<Pattern>
make_pattern(const Phrase& ph, Scope& scope, unsigned unitno)
{
    if (auto id = dynamic_cast<const Identifier*>(&ph)) {
        slot_t slot = scope.add_binding(id->atom_, ph, unitno);
        return make<Id_Pattern>(slot);
    }
    if (auto brackets = dynamic_cast<const Bracket_Phrase*>(&ph)) {
        std::vector<Shared<Pattern>> items;
        each_item(*brackets->body_, [&](Phrase& item)->void {
            items.push_back(make_pattern(item, scope, unitno));
        });
        return make<List_Pattern>(items);
    }
/*
    if (auto parens = dynamic_cast<Paren_Phrase*>(&ph)) {
    }
 */
    throw Exception(At_Phrase(ph, scope), "not a pattern");
}

} // namespace curv
