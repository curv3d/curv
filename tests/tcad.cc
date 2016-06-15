#include <gtest/gtest.h>
#include <curv/parse.h>
#include <curv/eval.h>
#include <curv/exception.h>
#include <curv/phrase.h>
#include <curv/string.h>

using namespace std;
using namespace aux;
using namespace curv;

struct CString_Script : public curv::Script
{
    const char* buffer_;

    CString_Script(const char* name, const char* buffer)
    :
        curv::Script(name, buffer, buffer + strlen(buffer)),
        buffer_(buffer)
    {}
};

struct Evaluator
{
    Evaluator(const char* source)
    :
        script_(aux::make_shared<CString_Script>("", source)),
        failure_(nullptr),
        success_(nullptr)
    {
        try {
            auto phrase = curv::parse(*script_);
            if (phrase == nullptr) { // empty expression
                success_ = "";
                return;
            }
            const curv::Definition* def =
                dynamic_cast<curv::Definition*>(phrase.get());
            if (def != nullptr) {
                failure_ = "definition found; expecting expression";
                return;
            }
            success_value_ = curv::eval(*phrase, curv::builtin_namespace);
            success_str_ = curv::stringify(success_value_);
            success_ = success_str_->c_str();
        } catch (curv::Exception& e) {
            failure_str_ = e.shared_what();
            failure_ = failure_str_->c_str();
        } catch (std::exception& e) {
            failure_str_ = mk_string(e.what());
            failure_ = failure_str_->c_str();
        }
    }

    aux::Shared_Ptr<CString_Script> script_;

    const char* failure_;
    aux::Shared_Ptr<curv::String> failure_str_;

    const char* success_;
    curv::Value success_value_;
    aux::Shared_Ptr<curv::String> success_str_;
};

testing::AssertionResult
evals_to(const char* e1, const char* e2, const char* expr, const char* expected)
{
    Evaluator result(expr);
    if (result.failure_) {
        return testing::AssertionFailure()
            << "in expr '" << expr << "'\n"
            << "expected: '" << expected << "'\n"
            << "  failed: '" << result.failure_ << "'\n";
    } else if (strcmp(expected, result.success_) == 0) {
        return testing::AssertionSuccess();
    } else {
        return testing::AssertionFailure()
            << "in expr '" << expr << "'\n"
            << "expected: '" << expected << "'\n"
            << "  actual: '" << result.success_ << "'\n";
    }
}

#define EVALS_TO(expr,result) EXPECT_PRED_FORMAT2(evals_to,expr,result)

TEST(tcad, eval)
{
    EVALS_TO("42", "42");
}
