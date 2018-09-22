#include <gtest/gtest.h>
#include <libcurv/source.h>
#include <libcurv/scanner.h>
#include <libcurv/parser.h>
#include <libcurv/phrase.h>
#include <libcurv/program.h>
#include "sys.h"

using namespace curv;

Shared<const Phrase>
nub(const char* str)
{
    auto source = make<String_Source>("", str);
    Scanner scanner{source, sys};
    auto phrase = parse_program(scanner);
    return nub_phrase(phrase);
}

Value
skip_prefix(const char* src, unsigned len)
{
    auto source = make<String_Source>("", src);
    Program prog{source, sys, Program_Opts().skip_prefix(len)};
    prog.compile();
    return prog.eval();
}

TEST(curv, program)
{
    ASSERT_TRUE(isa<const Identifier>(nub("foo")));
    ASSERT_TRUE(isa<const Identifier>(nub("(foo)")));
    ASSERT_TRUE(isa<const Identifier>(nub("let i=0 in (foo)")));
    ASSERT_TRUE(isa<const Identifier>(nub("let i=0 in (foo) where j=0")));

    ASSERT_TRUE(skip_prefix("-foo=42",5).get_num_or_nan() == 42.0);
/*
    auto xp = List::make(2);
    auto x = Shared<List>{std::move(xp)};

    auto yp = List::make(2);
    auto y = Shared<List>{std::move(yp)};

    (*x)[0] = Value{42.0};
    (*x)[1] = Value{y};

    ASSERT_EQ(x->size(), 2u);
    ASSERT_EQ((*x)[0], Value{42.0});
    ASSERT_EQ(x->use_count, 1u);
    ASSERT_EQ(y->use_count, 2u);
    x = nullptr;
    ASSERT_EQ(y->use_count, 1u);
*/
}
