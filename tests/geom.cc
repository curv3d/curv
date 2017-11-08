#include <gtest/gtest.h>
#include <cstdlib>

TEST(curv, geom)
{
    EXPECT_EQ(std::system("sh geom.sh"), 0);
}
