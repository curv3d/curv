// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/pattern.h>
#include <curv/phrase.h>
#include <curv/definition.h>
#include <curv/exception.h>
#include <curv/context.h>
#include <curv/gl_context.h>

namespace curv {

struct Id_Pattern : public Pattern
{
    slot_t slot_;

    Id_Pattern(Shared<const Phrase> s, slot_t i) : Pattern(s), slot_(i) {}

    virtual void analyze(Environ&) override
    {
    }
    virtual void exec(
        Value* slots, Value value, const Context&, Frame&) override
    {
        slots[slot_] = value;
    }
    virtual void gl_exec(GL_Value value, const Context&, GL_Frame& f) override
    {
        f[slot_] = value;
    }
};

struct List_Pattern : public Pattern
{
    std::vector<Shared<Pattern>> items_;

    List_Pattern(Shared<const Phrase> s, std::vector<Shared<Pattern>> items)
    :
        Pattern(s),
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
#if 0
    virtual void gl_exec(
        GL_Value val, const Context& valcx, GL_Frame& f) override
    {
    }
#endif
};

struct Record_Pattern : public Pattern
{
    Atom_Map<Shared<Pattern>> fields_;

    Record_Pattern(Shared<const Phrase> s, Atom_Map<Shared<Pattern>> fields)
    :
        Pattern(s),
        fields_(std::move(fields))
    {}

    virtual void analyze(Environ&) override
    {
    }
    virtual void exec(
        Value* slots, Value val, const Context& valcx, Frame& f) override
    {
        auto record = val.to<Structure>(valcx);
        for (auto p : fields_) {
            if (record->hasfield(p.first)) {
                auto fval = record->getfield(p.first,{});
                p.second->exec(slots, fval, At_Field(p.first.data(), valcx), f);
            } else {
                throw Exception(valcx, stringify(
                    "record does not have a field named ",p.first));
            }
        }
        if (record->size() != fields_.size())
            throw Exception(valcx,
                "record has extra fields not matched by pattern");
    }
};

Shared<Pattern>
make_pattern(const Phrase& ph, Scope& scope, unsigned unitno)
{
    if (auto id = dynamic_cast<const Identifier*>(&ph)) {
        slot_t slot = scope.add_binding(id->atom_, ph, unitno);
        return make<Id_Pattern>(share(ph), slot);
    }
    if (auto brackets = dynamic_cast<const Bracket_Phrase*>(&ph)) {
        std::vector<Shared<Pattern>> items;
        each_item(*brackets->body_, [&](Phrase& item)->void {
            items.push_back(make_pattern(item, scope, unitno));
        });
        return make<List_Pattern>(share(ph), items);
    }
    if (auto parens = dynamic_cast<const Paren_Phrase*>(&ph)) {
        std::vector<Shared<Pattern>> items;
        if (dynamic_cast<const Empty_Phrase*>(parens) != nullptr)
            return make<List_Pattern>(share(ph), items);
        if (dynamic_cast<const Comma_Phrase*>(&*parens->body_) == nullptr
         && dynamic_cast<const Semicolon_Phrase*>(&*parens->body_) == nullptr)
            return make_pattern(*parens->body_, scope, unitno);
        each_item(*parens->body_, [&](Phrase& item)->void {
            items.push_back(make_pattern(item, scope, unitno));
        });
        return make<List_Pattern>(share(ph), items);
    }
    if (auto braces = dynamic_cast<const Brace_Phrase*>(&ph)) {
        Atom_Map<Shared<Pattern>> fields;
        each_item(*braces->body_, [&](Phrase& item)->void {
            if (auto id = dynamic_cast<const Identifier*>(&item)) {
                fields[id->atom_] = make_pattern(*id, scope, unitno);
                return;
            }
            if (auto bin = dynamic_cast<const Binary_Phrase*>(&item)) {
                if (bin->op_.kind_ == Token::k_colon) {
                }
            }
            throw Exception(At_Phrase(item, scope), "not a field pattern");
        });
        return make<Record_Pattern>(share(ph), fields);
    }
    throw Exception(At_Phrase(ph, scope), "not a pattern");
}

void
Pattern::gl_exec(GL_Value val, const Context& valcx, GL_Frame& f)
{
    throw Exception(At_GL_Phrase(*source_, &f),
        "pattern not supported by Geometry Compiler");
}

} // namespace curv
