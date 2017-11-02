#include <gtest/gtest.h>
#include <cstdlib>

TEST(curv, geom)
{
    EXPECT_EQ(std::system("cd ../examples; sh test"), 0);
}
