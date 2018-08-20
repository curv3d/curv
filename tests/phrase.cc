#include <gtest/gtest.h>
#include <libcurv/source.h>
#include <libcurv/scanner.h>
#include <libcurv/parser.h>
#include <libcurv/phrase.h>

using namespace std;
using namespace curv;

Shared<const Phrase>
nub(const char* str)
{
    auto source = make<Source_String>(make_string(""), make_string(str));
    Scanner scanner{*source, nullptr};
    auto phrase = parse_program(scanner);
    return nub_phrase(phrase);
}

TEST(curv, phrase)
{
    ASSERT_TRUE(isa<const Identifier>(nub("foo")));
    ASSERT_TRUE(isa<const Identifier>(nub("(foo)")));
    ASSERT_TRUE(isa<const Identifier>(nub("let i=0 in (foo)")));
    ASSERT_TRUE(isa<const Identifier>(nub("let i=0 in (foo) where j=0")));
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
