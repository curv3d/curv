// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

extern "C" {
#include "readlinex.h"
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
}
#include <iostream>
#include <fstream>

#include "export.h"
#include "progdir.h"
#include "cscript.h"
#include <libcurv/geom/tempfile.h>
#include <libcurv/dtostr.h>
#include <libcurv/analyser.h>
#include <libcurv/context.h>
#include <libcurv/program.h>
#include <libcurv/exception.h>
#include <libcurv/file.h>
#include <libcurv/parser.h>
#include <libcurv/phrase.h>
#include <libcurv/shared.h>
#include <libcurv/system.h>
#include <libcurv/list.h>
#include <libcurv/record.h>
#include <libcurv/version.h>
#include <libcurv/die.h>
#include <libcurv/geom/export_frag.h>
#include <libcurv/geom/shape.h>
#include <libcurv/geom/viewer/viewer.h>

curv::geom::viewer::Viewer view;

bool view_shape(curv::Value value,
    curv::System& sys, const curv::Context &cx, bool block = false)
{
    curv::geom::Shape_Recognizer shape(cx, sys);
    if (shape.recognize(value)) {
        if (shape.is_2d_) std::cerr << "2D";
        if (shape.is_2d_ && shape.is_3d_) std::cerr << "/";
        if (shape.is_3d_) std::cerr << "3D";
        std::cerr << " shape "
            << (shape.bbox_.xmax - shape.bbox_.xmin) << "×"
            << (shape.bbox_.ymax - shape.bbox_.ymin);
        if (shape.is_3d_)
            std::cerr << "×" << (shape.bbox_.zmax - shape.bbox_.zmin);
        std::cerr << "\n";

        view.set_shape(shape);
        if (block)
            view.run();
        else
            view.open();
        return true;
    } else
        return false;
}

bool was_interrupted = false;

void interrupt_handler(int)
{
    was_interrupted = true;
}

void interactive_mode(curv::System& sys)
{
    // Catch keyboard interrupts, and set was_interrupted = true.
    // TODO: This will be used to interrupt the evaluator.
    struct sigaction interrupt_action;
    memset((void*)&interrupt_action, 0, sizeof(interrupt_action));
    interrupt_action.sa_handler = interrupt_handler;
    sigaction(SIGINT, &interrupt_action, nullptr);

    // top level definitions, extended by typing 'id = expr'
    curv::Namespace names = sys.std_namespace();

    for (;;) {
        // Race condition on assignment to was_interrupted.
        was_interrupted = false;
        RLXResult result;
        char* line = readlinex("curv> ", &result);
        if (line == nullptr) {
            std::cout << "\n";
            if (result == rlx_interrupt) {
                continue;
            }
            break;
        }
        auto script = curv::make<CString_Script>("", line);
        try {
            curv::Program prog{*script, sys};
            prog.compile(&names, nullptr);
            auto den = prog.denotes();
            if (den.first) {
                for (auto f : *den.first)
                    names[f.first] = curv::make<curv::Builtin_Value>(f.second);
            }
            if (den.second) {
                bool is_shape = false;
                if (den.second->size() == 1) {
                    static curv::Symbol lastval_key = "_";
                    names[lastval_key] =
                        curv::make<curv::Builtin_Value>(den.second->front());
                    is_shape = view_shape(den.second->front(),
                        sys, curv::At_Phrase(prog.nub(), nullptr));
                }
                if (!is_shape) {
                    for (auto e : *den.second)
                        std::cout << e << "\n";
                }
            }
        } catch (curv::Exception& e) {
            std::cout << "ERROR: " << e << "\n";
        } catch (std::exception& e) {
            std::cout << "ERROR: " << e.what() << "\n";
        }
    }
    view.close();
}
