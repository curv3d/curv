// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_PROGRAM_H
#define CURV_PROGRAM_H

#include <curv/builtin.h>
#include <curv/frame.h>
#include <curv/meaning.h>
#include <curv/module.h>
#include <curv/script.h>
#include <curv/shared.h>
#include <curv/system.h>
#include <curv/list.h>

namespace curv {

struct Program
{
    const Script& script_;
    System& system_;
    const Namespace* names_ = nullptr;
    Frame *parent_frame_ = nullptr;
    Shared<Phrase> phrase_ = nullptr;
    Shared<Meaning> meaning_ = nullptr;
    Shared<Module_Expr> module_ = nullptr;
    std::unique_ptr<Frame> frame_ = nullptr;

    Program(
        const Script& script,
        System& system)
    :
        script_(script),
        system_(system)
    {}

    void compile(
        const Namespace* names = nullptr,
        Frame *parent_frame = nullptr);

    const Phrase& value_phrase();

    std::pair<Shared<Module>, Shared<List>> denotes();

    Value eval();
};

} // namespace curv
#endif // header guard
