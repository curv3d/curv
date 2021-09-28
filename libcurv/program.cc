// Copyright 2016-2021 Doug Moen
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

struct File_Phrase : public Phrase
{
    Src_Loc loc_;
    File_Phrase(const Filesystem::path& path, Source::Type type)
    :
        loc_{make<Source>(path.string(), type), Token{}}
    {
    }
    virtual Src_Loc location() const override { return loc_; }
    virtual Shared<Meaning> analyse(Environ& env, Interp) const override
    {
        throw Exception(At_Phrase(*this, env),
            "internal error: File_Phrase::analyse");
    }
};

void
Program::compile(Shared<const Source> source, Scanner_Opts scopts,
    Environ& env, Interp terp)
{
    Scanner scanner(move(source), sstate_, scopts);
    phrase_ = parse_program(scanner);
    if (auto def = phrase_->as_definition(env, Fail::soft)) {
        module_ = analyse_module(*def, env);
    } else {
        meaning_ = phrase_->analyse(env, terp);
    }
    frame_ = {Frame::make(env.frame_maxslots_, sstate_, sstate_.file_frame_,
        nullptr, nullptr)};
}

void
Program::compile(Shared<const Source> source, Environ& env, Interp terp)
{
    compile(move(source), Scanner_Opts(), env, terp);
}

void
Program::compile(Shared<const Source> source)
{
    Builtin_Environ env{sstate_.system_.std_namespace(), sstate_};
    compile(move(source), env, Interp::expr());
}

void
Program::compile(Shared<const Source> source, Scanner_Opts scopts)
{
    Builtin_Environ env{sstate_.system_.std_namespace(), sstate_};
    compile(move(source), scopts, env, Interp::expr());
}

void
Program::compile(Filesystem::path path, Source::Type type, Value val)
{
    value_ = val;
    phrase_ = make<File_Phrase>(path, type);
}

const Phrase&
Program::syntax()
const
{
    return *nub_phrase(phrase_);
}

Value
Program::eval()
{
    if (value_)
        return value_;
    else if (module_ != nullptr) {
        throw Exception(At_Program(*this),
            "definition found; expecting an expression");
    } else {
        auto expr = meaning_->to_operation(sstate_);
        frame_->next_op_ = &*expr;
        return tail_eval_frame(move(frame_));
    }
}

Shared<Module>
Program::exec(Operation::Executor& ex)
{
    if (value_) {
        ex.push_value(value_, At_Program(*this));
        return nullptr;
    } else if (module_) {
        return module_->eval_module(*frame_);
    } else {
        auto op = meaning_->to_operation(sstate_);
        op->exec(*frame_, ex);
        return nullptr;
    }
}

} // namespace curv
