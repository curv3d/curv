#include <gtest/gtest.h>
#include <libcurv/tail_array.h>
#include <vector>
#include <memory>

using namespace std;
using namespace curv;

struct A
{
    TAIL_ARRAY_MEMBERS(double)
};
struct TA : public Tail_Array<A>
{
    using Tail_Array<A>::Tail_Array;
};

TEST(curv, tail_array)
{
    unique_ptr<TA> a{make_tail_array<TA>(3)};
    ASSERT_EQ(a->size(), 3u);

    std::vector<double> v;
    v.push_back(0);
    v.push_back(1);
    auto b = copy_tail_array<TA>(v.data(), v.size());
    ASSERT_TRUE(b->size() == 2);
    ASSERT_TRUE(b->begin()[0] == 0.0);
    ASSERT_TRUE(b->begin()[1] == 1.0);
    TA* p = b.release();
    delete p; // ensure that `delete` works on raw Tail_Array pointers.

    auto x = make_tail_array<TA>({0.0,1.0});
    ASSERT_TRUE(x->size() == 2);
    ASSERT_TRUE(x->begin()[0] == 0.0);
    ASSERT_TRUE(x->begin()[1] == 1.0);
}
