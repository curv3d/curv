// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/analyzer.h>
#include <curv/builtin.h>
#include <curv/eval.h>
#include <curv/parse.h>
#include <curv/scanner.h>
#include <curv/system.h>
#include <curv/exception.h>
#include <curv/context.h>

namespace curv {

void
Eval::compile(const Namespace* names, Frame* parent_frame)
{
    if (names == nullptr)
        names_ = &system_.std_namespace();
    else
        names_ = names;
    parent_frame_ = parent_frame;

    Scanner scanner{script_, parent_frame};
    phrase_ = parse_program(scanner);

    Builtin_Environ env{*names_, parent_frame};
    if (auto def = phrase_->analyze_def(env)) {
        Bindings_Analyzer fields{env};
        fields.add_statement(phrase_);
        fields.analyze(phrase_);
        module_ = make<Module_Expr>(phrase_,
            std::move(fields.defn_dictionary_),
            std::move(fields.bindings_));
    } else {
        meaning_ = phrase_->analyze(env);
    }

    frame_ = {Frame::make(env.frame_maxslots_,
        system_, parent_frame, nullptr, nullptr)};
}

const Phrase&
Eval::value_phrase()
{
    auto ph = phrase_;
    for (;;) {
        if (auto pr = cast<const Program_Phrase>(ph)) {
            ph = pr->body_;
            continue;
        }
        if (auto s = cast<const Semicolon_Phrase>(ph)) {
            ph = s->args_.back().expr_;
            continue;
        }
        if (auto p = cast<const Paren_Phrase>(ph)) {
            if (isa<Empty_Phrase>(p->body_))
                break;
            if (isa<Comma_Phrase>(p->body_))
                break;
            ph = p->body_;
            continue;
        }
        break;
    }
    return *ph;
}

Value
Eval::eval()
{
    if (module_ != nullptr) {
        throw Exception(At_Phrase(*phrase_, parent_frame_),
            "definition found; expecting an expression");
    } else {
        auto expr = meaning_->to_operation(parent_frame_);
        return expr->eval(*frame_);
    }
}

std::pair<Shared<Module>, Shared<List>>
Eval::denotes()
{
    Shared<Module> module = nullptr;
    Shared<List> list = nullptr;
    if (module_) {
        module = module_->eval_module(*frame_);
    } else {
        List_Builder lb;
        auto gen = meaning_->to_operation(parent_frame_);
        gen->generate(*frame_, lb);
        list = lb.get_list();
    }
    return {module, list};
}

} // namespace curv
