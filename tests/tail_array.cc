#include <gtest/gtest.h>
#include <aux/tail_array.h>
#include <aux/array_mixin.h>
#include <vector>
#include <memory>

using namespace std;
using namespace aux;

struct A : public Tail_Array_Data<double>
{
};
using TA = aux::Tail_Array<A>;

TEST(aux, tail_array)
{
    unique_ptr<TA> a{TA::make(3)};
    ASSERT_EQ(a->size(), 3);

    std::vector<double> v;
    v.push_back(0);
    v.push_back(1);
    auto b = TA::make_copy(v.data(), v.size());
    ASSERT_TRUE(b->size() == 2);
    ASSERT_TRUE(b->begin()[0] == 0.0);
    ASSERT_TRUE(b->begin()[1] == 1.0);
    TA* p = b.release();
    delete p; // ensure that `delete` works on raw Tail_Array pointers.

    auto x = TA::make({0.0,1.0});
    ASSERT_TRUE(x->size() == 2);
    ASSERT_TRUE(x->begin()[0] == 0.0);
    ASSERT_TRUE(x->begin()[1] == 1.0);
}
