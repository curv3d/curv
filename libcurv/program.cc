// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/program.h>

#include <libcurv/analyser.h>
#include <libcurv/builtin.h>
#include <libcurv/context.h>
#include <libcurv/definition.h>
#include <libcurv/exception.h>
#include <libcurv/parser.h>
#include <libcurv/scanner.h>
#include <libcurv/system.h>

namespace curv {

void
Program::compile(const Namespace* names)
{
    if (names == nullptr)
        names_ = &scanner_.system_.std_namespace();
    else
        names_ = names;

    phrase_ = parse_program(scanner_);

    Analyser ana(scanner_.system_);
    Builtin_Environ env{*names_, ana, scanner_.file_frame_};
    if (auto def = phrase_->as_definition(env)) {
        module_ = analyse_module(*def, env);
    } else {
        meaning_ = phrase_->analyse(env, 0);
    }

    frame_ = {Frame::make(env.frame_maxslots_,
        scanner_.system_, scanner_.file_frame_, nullptr, nullptr)};
}

const Phrase&
Program::nub()
const
{
    return *nub_phrase(phrase_);
}

Location
Program::location()
const
{
    if (phrase_ == nullptr) {
        return Location{
            *scanner_.source_,
            Token{unsigned(scanner_.ptr_ - scanner_.source_->begin()),
                  unsigned(scanner_.source_->size())}};
    } else {
        return nub_phrase(phrase_)->location();
    }
}

Value
Program::eval()
{
    if (module_ != nullptr) {
        throw Exception(At_Phrase(*phrase_, scanner_),
            "definition found; expecting an expression");
    } else {
        auto expr = meaning_->to_operation(scanner_.system_,scanner_.file_frame_);
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
        Operation::List_Executor lex(lb);
        auto gen = meaning_->to_operation(scanner_.system_,scanner_.file_frame_);
        gen->exec(*frame_, lex);
        list = lb.get_list();
    }
    return {module, list};
}

} // namespace curv
