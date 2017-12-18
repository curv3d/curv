// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/analyser.h>
#include <curv/definition.h>
#include <curv/builtin.h>
#include <curv/program.h>
#include <curv/parser.h>
#include <curv/scanner.h>
#include <curv/system.h>
#include <curv/exception.h>
#include <curv/context.h>

namespace curv {

void
Program::compile(const Namespace* names, Frame* parent_frame)
{
    if (names == nullptr)
        names_ = &system_.std_namespace();
    else
        names_ = names;
    parent_frame_ = parent_frame;

    Scanner scanner{script_, parent_frame};
    phrase_ = parse_program(scanner);

    Builtin_Environ env{*names_, system_, parent_frame};
    if (auto def = phrase_->as_definition(env)) {
        module_ = analyse_module(*def, env);
    } else {
        meaning_ = phrase_->analyse(env);
    }

    frame_ = {Frame::make(env.frame_maxslots_,
        system_, parent_frame, nullptr, nullptr)};
}

const Phrase&
Program::value_phrase()
{
    Shared<const Phrase> ph = phrase_;
    for (;;) {
        if (auto pr = cast<const Program_Phrase>(ph)) {
            ph = pr->body_;
            continue;
        }
        if (auto let = cast<const Let_Phrase>(ph)) {
            ph = let->body_;
            continue;
        }
        if (auto bin = cast<const Binary_Phrase>(ph)) {
            if (bin->op_.kind_ == Token::k_where) {
                ph = bin->right_;
                continue;
            }
            break;
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
Program::eval()
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
Program::denotes()
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
