#include <gtest/gtest.h>
#include <curv/parse.h>
#include <curv/eval.h>
#include <curv/exception.h>
#include <curv/phrase.h>
#include <curv/string.h>
#include <curv/analyzer.h>
#include <cstdlib>

using namespace std;
using namespace aux;
using namespace curv;

struct CString_Script : public curv::Script
{
    const char* buffer_;

    CString_Script(const char* name, const char* buffer)
    :
        curv::Script(mk_string(name), buffer, buffer + strlen(buffer)),
        buffer_(buffer)
    {}
};

struct Evaluator
{
    Evaluator(const char* source)
    :
        script_(aux::make_shared<CString_Script>("", source)),
        failmsg_(nullptr),
        failall_(nullptr),
        success_(nullptr)
    {
        try {
            Shared<Module> module{eval_script(*script_, builtin_namespace)};
            if (module->elements_->empty()) {
                success_ = "";
                return;
            }
            if (module->elements_->size() > 1) {
                failmsg_ = "multiple values found";
                failall_ = failmsg_;
                return;
            }
            success_value_ = (*module->elements_)[0];
            success_str_ = curv::stringify(success_value_);
            success_ = success_str_->c_str();
        } catch (curv::Exception& e) {
            failmsg_str_ = e.shared_what();
            failmsg_ = failmsg_str_->c_str();
            failall_str_ = stringify(e);
            failall_ = failall_str_->c_str();
        } catch (std::exception& e) {
            failmsg_str_ = mk_string(e.what());
            failmsg_ = failmsg_str_->c_str();
            failall_ = failmsg_;
        }
    }

    aux::Shared<CString_Script> script_;

    const char* failmsg_;
    aux::Shared<const curv::String> failmsg_str_;

    const char* failall_;
    aux::Shared<const curv::String> failall_str_;

    const char* success_;
    curv::Value success_value_;
    aux::Shared<curv::String> success_str_;
};

enum Expectation { x_success, x_failmsg, x_failall };

testing::AssertionResult
evaltest(const char* expr, const char* expected, Expectation expectation)
{
    Evaluator result(expr);
    if (result.failmsg_) {
        // the test failed
        switch (expectation) {
        case x_success:
            return testing::AssertionFailure()
                << "in expr '" << expr << "'\n"
                << "expected value: '" << expected << "'\n"
                << "  actual error: '" << result.failmsg_ << "'\n";
        case x_failmsg:
            if (strcmp(expected, result.failmsg_) == 0) {
                return testing::AssertionSuccess();
            } else {
                return testing::AssertionFailure()
                    << "in expr '" << expr << "'\n"
                    << "expected error: '" << expected << "'\n"
                    << "  actual error: '" << result.failmsg_ << "'\n";
            }
        case x_failall:
            if (strcmp(expected, result.failall_) == 0) {
                return testing::AssertionSuccess();
            } else {
                return testing::AssertionFailure()
                    << "in expr '" << expr << "'\n"
                    << "expected error: '" << expected << "'\n"
                    << "  actual error: '" << result.failall_ << "'\n";
            }
        default:
            assert(0);
        }
    } else {
        // the test succeeded
        switch (expectation) {
        case x_success:
            if (strcmp(expected, result.success_) == 0) {
                return testing::AssertionSuccess();
            } else {
                return testing::AssertionFailure()
                    << "in expr '" << expr << "'\n"
                    << "expected value: '" << expected << "'\n"
                    << "  actual value: '" << result.success_ << "'\n";
            }
        case x_failmsg:
        case x_failall:
            return testing::AssertionFailure()
                << "in expr '" << expr << "'\n"
                << "expected error: '" << expected << "'\n"
                << "  actual value: '" << result.success_ << "'\n";
        default:
            assert(0);
        }
    }
}

testing::AssertionResult
eval_success(
    const char* e1, const char* e2,
    const char* expr, const char* expected)
{
    return evaltest(expr, expected, x_success);
}

testing::AssertionResult
eval_failmsg(
    const char* e1, const char* e2,
    const char* expr, const char* expected)
{
    return evaltest(expr, expected, x_failmsg);
}

testing::AssertionResult
eval_failall(
    const char* e1, const char* e2,
    const char* expr, const char* expected)
{
    return evaltest(expr, expected, x_failall);
}

int reps()
{
    const char* rs = getenv("REPS");
    if (rs == nullptr)
        return 1;
    int r = atoi(rs);
    if (r <= 0)
        return 1;
    return r;
}

#define SUCCESS(expr,result) EXPECT_PRED_FORMAT2(eval_success,expr,result)
#define FAILMSG(expr,result) EXPECT_PRED_FORMAT2(eval_failmsg,expr,result)
#define FAILALL(expr,result) EXPECT_PRED_FORMAT2(eval_failall,expr,result)

TEST(curv, eval)
{
  int r = reps();
  for (int i = 0; i < r; ++i) {
    // constructors
    SUCCESS("42.7", "42.7");
    SUCCESS(".1", "0.1");
    SUCCESS("1.", "1");
    SUCCESS(".1e-1", "0.01");
    SUCCESS("1.e+1", "10");
    SUCCESS("1e1", "10");
    SUCCESS("\"abc\"", "\"abc\"");
    SUCCESS("[1,2,3]", "[1,2,3]");
    SUCCESS("{x=1}", "{x=1}");

    // builtins
    SUCCESS("pi",  "3.141592653589793");
    SUCCESS("tau", "6.283185307179586");
    SUCCESS("inf", "inf");
    SUCCESS("null", "null");
    SUCCESS("false", "false");
    SUCCESS("true", "true");
    SUCCESS("sqrt", "<function>");

    // runtime operations
    SUCCESS("-0", "-0");
    SUCCESS("-inf", "-inf");
    FAILMSG("1+null", "1+null: domain error");
    FAILMSG("0/0", "0/0: domain error");
    SUCCESS("1/0", "inf");
    SUCCESS("sqrt(2)", "1.4142135623730951");
    SUCCESS("sqrt(4,)", "2"); // test syntax: trailing , after last argument
    SUCCESS("sqrt sqrt 16", "2");
    FAILALL("f=()->sqrt(true);\nf()",
        "sqrt(true): domain error\n"
        "line 1(columns 12-15)\n"
        "  f=()->sqrt(true);\n"
        "             ^---  \n"
        "line 2(columns 1-3)\n"
        "  f()\n"
        "  ^--");
    SUCCESS("len[]", "0");
    FAILALL("len 0",
        "not a list\n"
        "line 1(column 5)\n"
        "  len 0\n"
        "      ^");
    SUCCESS("true||false", "true");
    SUCCESS("false||true", "true");
    SUCCESS("false||false", "false");
    SUCCESS("true||null", "true");
    FAILMSG("null||true", "not a boolean value");

    SUCCESS("false&&true", "false");
    SUCCESS("false&&null", "false");
    SUCCESS("true&&false", "false");
    FAILMSG("true&&null", "not a boolean value");
    SUCCESS("true&&true", "true");

    FAILMSG("len(if (true) [])",
        "if: not an expression (missing else clause)");
    SUCCESS("1 + if (true) 2 else 5", "3");

    SUCCESS("null==null", "true");
    SUCCESS("null==false", "false");
    SUCCESS("false==false", "true");
    SUCCESS("42==42.0", "true");
    SUCCESS("0==false", "false");
    SUCCESS("[1,2]==[1,2]", "true");
    SUCCESS("[1,true]==[1,2]", "false");
    SUCCESS("{x=1,y=2}=={x=1,y=2}", "true");
    SUCCESS("sqrt==sqrt", "true");
    SUCCESS("!true", "false");
    SUCCESS("!false", "true");
    FAILMSG("!null", "!null: domain error");
    SUCCESS("null!=null", "false");
    SUCCESS("null!=false", "true");
    SUCCESS("0 < 1", "true");
    SUCCESS("-0 < +0", "false");
    FAILMSG("0 < null", "0<null: domain error");
    SUCCESS("0 <= 1", "true");
    SUCCESS("1 > 0", "true");
    SUCCESS("1 >= 0", "true");
    SUCCESS("{f=sqrt}.f(4)", "2");
    SUCCESS("4^0.5", "2");
    SUCCESS("4^-1", "0.25");
    SUCCESS("-2^2", "-4");
    SUCCESS("[1,2,3].[1]","2");
    FAILALL("[1,2,3].[1.1]",
        "not an integer\n"
        "line 1(columns 10-12)\n"
        "  [1,2,3].[1.1]\n"
        "           ^-- ");
    SUCCESS("x+y;x=1;y=2", "3");
    SUCCESS("a=c+1;b=1;c=b+1;a", "3");
    FAILMSG("x=x;x", "illegal recursive reference");
    SUCCESS("x=1;let(y=2)let(z=3)x+y+z", "6");
    FAILALL("let(x=x)x",
        "illegal recursive reference\n"
        "line 1(column 7)\n"
        "  let(x=x)x\n"
        "        ^  ");
    SUCCESS("f=x->let(a=x+1)a; f 2", "3");
    FAILMSG("f=x->x; f()", "wrong number of arguments");
    SUCCESS("add=(x,y)->x+y;add(1,2)", "3");
    SUCCESS("add=x->y->x+y;add(1)(2)", "3");
    SUCCESS(
        "sum = (list,i,f)->if (i < len list) list.[i]+f(list,i+1,f) else 0;"
        "sum([1,2,3],0,sum)",
        "6");
    SUCCESS(
        "/* tail-recursive function */"
        "sum = (list,i)->if (i < len list) list.[i]+sum(list,i+1) else 0;"
        "sum([1,2,3],0)",
        "6");
    SUCCESS(
        "// factorial (non-tail-recursive function)\n"
        "f = x->if (x <= 1) 1 else x * f(x-1);\n"
        "f(3)",
        "6");
    FAILALL("f=x->x x; f 0",
        "0: not a function\n"
        "line 1(column 6)\n"
        "  f=x->x x; f 0\n"
        "       ^       \n"
        "line 1(columns 11-13)\n"
        "  f=x->x x; f 0\n"
        "            ^--");

    // file
    FAILALL("file(\"bad_token.curv\")",
        "unterminated comment\n"
        "file \"bad_token.curv\", lines 1(column 5)-2(column 3)\n"
        "  x + /********\n"
        "      ^--------\n"
        "line 1(columns 1-22)\n"
        "  file(\"bad_token.curv\")\n"
        "  ^---------------------");
    FAILALL("file[\n1,2];",
        "not a string\n"
        "lines 1(column 5)-2(column 4)\n"
        "  file[\n"
        "      ^");
    FAILALL("file \"nonexistent\"",
        "can't open file nonexistent\n"
        "line 1(columns 1-18)\n"
        "  file \"nonexistent\"\n"
        "  ^-----------------");
    SUCCESS("std = file \"std.curv\"; std.concat[[1], [2,3], [4]]",
        "[1,2,3,4]");

    // for
    FAILMSG("for", "missing argument following 'for'");
    FAILMSG("for (i=a)", "missing expression");
    FAILMSG("for x x", "for: malformed argument");
    FAILMSG("for () x", "for: malformed argument");
    FAILMSG("for (i=a,j=b) x", "for: malformed argument");
    FAILMSG("for (i) x", "for: not a definition");
    FAILMSG("for (42=i) x", "for: not an identifier");
    SUCCESS("[for (i=[1,2,3]) i+1]", "[2,3,4]");

    // sequence generator
    SUCCESS("[for (i=[1,2,3]) if (i==2) (\"two\", \"2!\") else i]", 
        "[1,\"two\",\"2!\",3]");

    // lexical errors
    FAILMSG("\\foo", "illegal character '\\'");
    FAILMSG("\177", "illegal character 0x7F");
    FAILMSG("42e+", "bad numeral");
    FAILALL("/* foo",
        "unterminated comment\n"
        "line 1(columns 1-6)\n"
        "  /* foo\n"
        "  ^-----");

    // analysis errors
    FAILMSG("fnord", "fnord: not defined");
    FAILALL("{x=1,x=2}",
        "x: multiply defined\n"
        "line 1(column 6)\n"
        "  {x=1,x=2}\n"
        "       ^   ");
    FAILALL("x+",
        "missing expression\n"
        "line 1(column 3), at end of script\n"
        "  x+\n"
        "    ^");
    FAILALL("x+\n",
        "missing expression\n"
        "line 1(column 3), at end of script\n"
        "  x+\n"
        "    ^");
    FAILMSG("(a=0)+1", "not an operation");
  }
}
