#include <gtest/gtest.h>
#include <aux/tail_array.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cmath>

using namespace std;
using namespace aux;

struct A
{
    using value_type = double;
    short size_;
    double array_[0];
};

TEST(aux, tail_array)
{
    using TA = aux::Tail_Array<A>;
    TA *a = TA::make(3);
    ASSERT_TRUE(a->size_ == 3);
    delete a;
}
