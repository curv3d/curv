// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

//----------------------------------------------------------------------------
// The REPL is the Read-Eval-Print Loop. It's the interactive command line
// shell that lets you type Curv expressions, actions and definitions.
//
// It has two threads: a Repl thread that runs the REPL, and a Viewer thread
// that displays shapes in a Viewer window, and runs the OpenGL frame loop.
// The Viewer thread is a "worker" thread that receives messages from
// the REPL thread telling it to display a new shape or exit.
//
// On macOS, windows can only be run in the main thread, so the Viewer thread
// must be the main thread, and we must spawn a new thread to run the REPL.
// This is an inversion of the usual practice of running workers in spawned
// threads.

extern "C" {
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
}
#include <iostream>
#include <fstream>
#include <thread>
#include <condition_variable>
#include <functional>

#include <replxx.hxx>

#include "shapes.h"
#include "view_server.h"

#include <libcurv/analyser.h>
#include <libcurv/ansi_colour.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/program.h>
#include <libcurv/shape.h>
#include <libcurv/source.h>
#include <libcurv/system.h>

#include <libcurv/viewer/viewer.h>

#ifdef _WIN32
    #undef ERROR
#endif

using namespace curv;
using replxx::Replxx;

View_Server view_server;

bool was_interrupted = false;

#ifndef _WIN32
    // Interrupt (signal) handler (e.g. for Ctrl-C breaks) in the REPL set up by the function `repl` below.
    // So far interrupt handling has only been implemented for *nix-based OSes.
    void interrupt_handler(int)
    {
        was_interrupted = true;
    }
#endif

/// The meaning of a call to `help`, such as `help "foo"`.
struct Help_Action : public Just_Action
{
    Shared<Meaning> arg_;
    Help_Action(
        Shared<const Phrase> syntax,
        Shared<Meaning> arg)
    :
        Just_Action(move(syntax)),
        arg_(move(arg))
    {}
    virtual void exec(Frame& fm, Executor&) const override
    {
        if (auto mf = cast<Metafunction>(arg_)) {
            mf->print_help(fm.sstate_.system_.console());
        } else if (auto op = cast<Operation>(arg_)) {
            Value arg = op->eval(fm);
            help(arg, fm.sstate_.system_.console());
        } else {
            throw Exception(At_Phrase(*arg_->syntax_, fm), "Bad argument to 'help'");
        }
    }
};
/// The meaning of the phrase `help` in isolation.
struct Help_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    Shared<Operation> to_operation(Source_State& sstate) override
    {
        throw Exception(At_Phrase(*syntax_, sstate),
            "Usage: help <expression>");
    }
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        auto arg = ph.arg_->analyse(env, Interp::expr());
        return make<Help_Action>(share(ph), arg);
    }
    void print_help(std::ostream& out) const override
    {
        out <<
          "help <expression>\n"
          "  Print help information for the value returned by <expression>.\n";
    }
};

struct REPL_Namespace
{
    Namespace global_;
    Symbol_Map<Value> local_;
    REPL_Namespace(System& sys)
    :
        global_(sys.std_namespace()),
        local_{}
    {
        global_[make_symbol("help")] =
            make<Builtin_Meaning<Help_Metafunction>>();
    }

    std::vector<Replxx::Completion> completions(
        std::string const& input, int& contextLen)
    {
        // TODO: Scan input using scanner. Only return completions if the final
        // token is an identifier. (Don't complete inside strings or comments.)
        // TODO: Match local variables as well (which may be quoted, see next)
        // TODO: Work correctly for quoted identifiers.

        // Scan input, see if the final token is a non-zero-length identifier.
        // The length of this final identifier (or 0) is the contextLen.
        contextLen = 0;
        for (int i = int(input.size())-1; i >= 0; --i) {
            if (isalnum(input[i]) || input[i] == '_')
                ++contextLen;
            else break;
        }

        std::vector<Replxx::Completion> result;
        if (contextLen == 0) return result;
        const char* prefix = input.c_str() + input.size() - contextLen;

        for (auto const& n : global_) {
            if (int(n.first.size()) < contextLen)
                continue;
            if (memcmp(prefix, n.first.c_str(), contextLen) == 0)
                result.emplace_back(n.first.c_str());
        }
        return result;
    }

    void set_last_value(Value val)
    {
        static Symbol_Ref lastval_key = make_symbol("_");
        local_[lastval_key] = val;
    }

    void define(Symbol_Ref name, Value val)
    {
        local_[name] = val;
    }
};

struct REPL_Locative : public Locative
{
    Value* ptr_;
    REPL_Locative(Shared<const Phrase> syntax, Value* ptr)
    :
        Locative(syntax),
        ptr_(ptr)
    {}

    virtual Value fetch(Frame&) const override
    {
        return *ptr_;
    }
    virtual void store(Frame&, Value newval, const At_Syntax&) const override
    {
        *ptr_ = newval;
    }
};

struct REPL_Environ : public Environ
{
    REPL_Namespace& names_;
    REPL_Environ(REPL_Namespace& n, Source_State& a)
    :
        Environ(a),
        names_(n)
    {}
    virtual Shared<Meaning> single_lookup(
        const Identifier& id) override
    {
        auto pl = names_.local_.find(id.symbol_);
        if (pl != names_.local_.end())
            return make<Constant>(share(id), pl->second);
        auto pg = names_.global_.find(id.symbol_);
        if (pg != names_.global_.end())
            return pg->second->to_meaning(id);
        return nullptr;
    }
    virtual Unique<const Locative> single_lvar_lookup(const Identifier& id) override
    {
        auto pl = names_.local_.find(id.symbol_);
        if (pl != names_.local_.end())
            return make_unique<REPL_Locative>(share(id), &pl->second);
        auto pg = names_.global_.find(id.symbol_);
        if (pg != names_.global_.end())
            throw Exception(At_Phrase(id, *this),
                stringify(id.symbol_,": not assignable"));
        return nullptr;
    }
};

using Color = Replxx::Color;

void set_colour(Replxx::colors_t& colours, Token tok, Replxx::Color col)
{
    auto first_white = std::min(colours.size(), size_t(tok.first_white_));
    auto first = std::min(colours.size(), size_t(tok.first_));
    auto last = std::min(colours.size(), size_t(tok.last_));
    auto cp = colours.begin();
    std::fill(cp + first, cp + last, col);
    // colour comments
    std::fill(cp + first_white, cp + first, Color::GRAY);
}

void color_input(std::string const& context, Replxx::colors_t& colors,
    System* sys)
{
  auto source = make<String_Source>("", context);
  Source_State sstate{*sys, nullptr};
  Scanner scanner{std::move(source), sstate};

  // TODO: Use parser to parse the string, using a subclass of Scanner that
  // colours each token as it is read. This ensures that $(expr) substitutions
  // in string literals are coloured correctly.
  try {
    for (;;) {
      Color col;
      Token tok = scanner.get_token();

      if (tok.kind_ > Token::k_ident && tok.kind_ < Token::end_keywords)
        col = Color::BRIGHTMAGENTA;
      else
      switch (tok.kind_) {
        case Token::k_end:
          set_colour(colors, tok, Color::GRAY);
          return;

        case Token::k_bad_token:
        case Token::k_bad_utf8:
          col = Color::ERROR;
          break;

        // numerals
        case Token::k_num:
        case Token::k_hexnum:
        case Token::k_symbol:
          col = Color::BROWN;
          break;

        // string literal
        case Token::k_quote:
        case Token::k_string_segment:
        case Token::k_char_escape:
        case Token::k_backtick:
          col = Color::BRIGHTGREEN;
          break;

        // string interpolation
        case Token::k_dollar_paren:
        case Token::k_dollar_brace:
        case Token::k_dollar_bracket:
        case Token::k_dollar_ident:
          col = Color::BROWN;
          break;

        // syntactic symbols
        case Token::k_semicolon:
        case Token::k_equate:
        case Token::k_assign:
        case Token::k_ellipsis:
        case Token::k_at:
        case Token::k_right_arrow:
          col = Color::BRIGHTRED;
          break;

        // operators
        case Token::k_power:
        case Token::k_plus:
        case Token::k_plus_plus:
        case Token::k_minus:
        case Token::k_times:
        case Token::k_over:
        case Token::k_range:
        case Token::k_open_range:
        case Token::k_equal:
        case Token::k_not_equal:
        case Token::k_less:
        case Token::k_less_or_equal:
        case Token::k_greater:
        case Token::k_greater_or_equal:
        case Token::k_bang:
        case Token::k_and:
        case Token::k_or:
        case Token::k_left_call:
        case Token::k_right_call:
          col = Color::BRIGHTBLUE;
          break;

        default:
          col = Color::DEFAULT;
          break;
      }
      set_colour(colors, tok, col);
    }
  } catch (Exception& e) {
    auto tok = e.loc_.front().srcloc_.token();
    set_colour(colors, tok, Color::ERROR);
  }
}

// REPL_Executor is used to execute a command line program that is a statement.
// Both push_value() and push_field() are supported.
// In most cases, the value or field is printed on the console.
// If there is 1 push_value and no push_field calls, then I test if the value
// is a shape, and display the shape graphically.
//
// Future: With multi-viewer support, 'cube;sphere' could open multiple Viewers.
// I don't know how reuse of already open viewer windows would work.
struct REPL_Executor : public Operation::Executor
{
    REPL_Namespace& names_;
    Render_Opts render_;
    bool has_produced_output_ = false;
    Value only_output_so_far_is_this_value_ = missing;

    REPL_Executor(REPL_Namespace& n, Render_Opts r)
    :
        names_(n),
        render_(r)
    {}

    void start_command()
    {
        has_produced_output_ = false;
        only_output_so_far_is_this_value_ = missing;
    }
    void push_value(Value val, const Context&) override
    {
        if (!has_produced_output_) {
            only_output_so_far_is_this_value_ = val;
            has_produced_output_ = true;
        } else {
            if (!only_output_so_far_is_this_value_.eq(missing)) {
                std::cout << only_output_so_far_is_this_value_ << "\n";
                only_output_so_far_is_this_value_ = missing;
            }
            std::cout << val << "\n";
        }
    }
    void push_field(Symbol_Ref name, Value val, const Context&)
    override
    {
        if (!only_output_so_far_is_this_value_.eq(missing)) {
            std::cout << only_output_so_far_is_this_value_ << "\n";
            only_output_so_far_is_this_value_ = missing;
        }
        std::cout << name << ":" << val << "\n";
        has_produced_output_ = true;
    }
    void end_command(Program& prog)
    {
        if (!only_output_so_far_is_this_value_.eq(missing)) {
            Value val = only_output_so_far_is_this_value_;
            names_.set_last_value(val);
            Shape_Program shape{prog};
            if (shape.recognize(val, &render_)) {
                print_shape(shape);
                view_server.display_shape(shape, render_);
            } else {
                std::cout << val << "\n";
            }
        }
    }
};

void repl(System* sys, const Render_Opts* render)
{
#ifndef _WIN32
    // Catch keyboard interrupts, and set was_interrupted = true.
    // TODO: This will be used to interrupt the evaluator.
    if (isatty(0)) {
        struct sigaction interrupt_action;
        memset((void*)&interrupt_action, 0, sizeof(interrupt_action));
        interrupt_action.sa_handler = interrupt_handler;
        sigaction(SIGINT, &interrupt_action, nullptr);
    }
#endif

    // top level definitions, extended by typing 'id = expr'
    REPL_Namespace names{*sys};

    Replxx rx;
    rx.set_completion_callback(
        [&](std::string const& input, int& contextLen)
        -> Replxx::completions_t
        {
            return names.completions(input, contextLen);
        });
    rx.set_highlighter_callback(
        [&](std::string const& input, Replxx::colors_t& colors) -> void
        {
            color_input(input, colors, sys);
        });
    // use libedit compatible key bindings
    rx.bind_key_internal( Replxx::KEY::control( 'N' ), "history_next" );
    rx.bind_key_internal( Replxx::KEY::control( 'P' ), "history_previous" );

    REPL_Executor executor{names, *render};

    for (;;) {
        was_interrupted = false;
        const char* line = rx.input(AC_PROMPT "curv> " AC_RESET);
        if (line == nullptr) {
            if (errno == EAGAIN) continue;
            std::cout << "\n";
            break;
        }
        if (line[0] != '\0')
            rx.history_add(line);

        try {
            Program prog{*sys};
            auto source = make<String_Source>("", line);
            REPL_Environ env(names, prog.sstate_);
            prog.compile(source, env, Interp::stmt(1));
            executor.start_command();
            auto bindings = prog.exec(executor);
            executor.end_command(prog);
            if (bindings) {
                for (auto f : *bindings)
                    names.define(f.first, f.second);
            }
        } catch (std::exception& e) {
            sys->error(e);
        }
    }
    view_server.exit();
}

void interactive_mode(
    System& sys, const viewer::Viewer_Config& opts)
{
#ifndef _WIN32
    sys.use_colour_ = true;
#endif
    const Render_Opts *render = &opts;
    std::thread repl_thread(repl, &sys, render);
    view_server.run(opts);
    if (repl_thread.joinable())
        repl_thread.join();
}
