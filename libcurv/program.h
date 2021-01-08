// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PROGRAM_H
#define LIBCURV_PROGRAM_H

#include <libcurv/analyser.h>
#include <libcurv/builtin.h>
#include <libcurv/filesystem.h>
#include <libcurv/frame.h>
#include <libcurv/list.h>
#include <libcurv/meaning.h>
#include <libcurv/module.h>
#include <libcurv/scanner.h>
#include <libcurv/source.h>
#include <libcurv/system.h>

namespace curv {

struct Program_Opts
{
    Frame* file_frame_ = nullptr;
    Program_Opts& file_frame(Frame* f) { file_frame_=f; return *this; }

    unsigned skip_prefix_ = 0;
    Program_Opts& skip_prefix(unsigned n) { skip_prefix_=n; return *this; }
};

struct Program
{
    Scanner scanner_;
    Shared<Phrase> phrase_ = nullptr;
    Shared<Meaning> meaning_ = nullptr;
    Shared<Module_Expr> module_ = nullptr;
    Source_State sstate_;
    std::unique_ptr<Frame> frame_ = nullptr;
    Interp terp_ = Interp::expr();

    Program(
        Shared<const Source> source,
        System& system,
        Program_Opts opts = {})
    :
        scanner_(std::move(source), system, Scanner_Opts()
            .file_frame(opts.file_frame_)
            .skip_prefix(opts.skip_prefix_)),
        sstate_(system, opts.file_frame_)
    {}

    void skip_prefix(unsigned len);

    void compile(const Namespace* names = nullptr);
    void compile(Environ&);

    const Phrase& nub() const;

    Location location() const;
    System& system() const { return scanner_.system_; }
    Frame* file_frame() const { return scanner_.file_frame_; }

    Shared<Module> exec(Operation::Executor&);

    Value eval();
};

} // namespace curv
#endif // header guard
