// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PROGRAM_H
#define LIBCURV_PROGRAM_H

#include <libcurv/builtin.h>
#include <libcurv/frame.h>
#include <libcurv/meaning.h>
#include <libcurv/module.h>
#include <libcurv/scanner.h>
#include <libcurv/source.h>
#include <libcurv/shared.h>
#include <libcurv/system.h>
#include <libcurv/list.h>

namespace curv {

struct Program_Opts
{
    Frame* parent_frame_ = nullptr;
    Program_Opts& parent_frame(Frame* f) { parent_frame_=f; return *this; }

    unsigned skip_prefix_ = 0;
    Program_Opts& skip_prefix(unsigned n) { skip_prefix_=n; return *this; }
};

struct Program
{
    Scanner scanner_;
    System& system_;
    const Namespace* names_ = nullptr;
    Shared<Phrase> phrase_ = nullptr;
    Shared<Meaning> meaning_ = nullptr;
    Shared<Module_Expr> module_ = nullptr;
    std::unique_ptr<Frame> frame_ = nullptr;

    Program(
        Shared<const Source> source,
        System& system,
        Program_Opts opts = {})
    :
        scanner_(std::move(source), Scanner_Opts()
            .eval_frame(opts.parent_frame_)
            .skip_prefix(opts.skip_prefix_)),
        system_(system)
    {}

    void skip_prefix(unsigned len);

    void compile(const Namespace* names = nullptr);

    const Phrase& nub() const;

    std::pair<Shared<Module>, Shared<List>> denotes();

    Value eval();
};

} // namespace curv
#endif // header guard
