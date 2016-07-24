#include <gtest/gtest.h>
#include <aux/tail_array.h>
#include <aux/array_mixin.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <vector>

using namespace std;
using namespace aux;

struct A
{
    using value_type = double;
    short size_;
    double array_[0];
    A(size_t size) : size_(size) {}
    size_t size() const { return size_; }
    double* data() { return array_; }
    const double* data() const { return array_; }
};

TEST(aux, tail_array)
{
    class TA final : public aux::Tail_Array<aux::Array_Mixin<A>,TA> {};
    TA *a = TA::make(3);
    ASSERT_TRUE(a->size_ == 3);
    delete a;

    std::vector<double> v;
    v.push_back(0);
    v.push_back(1);
    TA* b = TA::make_copy(v.data(), v.size());
    ASSERT_TRUE(b->size_ == 2);
    ASSERT_TRUE(b->array_[0] == 0.0);
    ASSERT_TRUE(b->array_[1] == 1.0);
    delete b;
}
