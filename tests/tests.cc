#include <gtest/gtest.h>
#include <libcurv/io/tempfile.h>
extern "C" {
#include <stdlib.h>
}

int
main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    atexit(curv::geom::remove_all_tempfiles);
    return RUN_ALL_TESTS();
}
