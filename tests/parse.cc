#include <gtest/gtest.h>
#include <curv/location.h>

using namespace std;
using namespace aux;
using namespace curv;

TEST(curv, parse)
{
    auto s1 = make<String_Script>(make_string(""), make_string("x+\n"));
    Token t1;
    t1.first = 3;
    t1.last = 3;
    Location l1(*s1, t1);
    Location::Line_Info li1 = l1.line_info();

    ASSERT_EQ(li1.start_line_num, 0);
    ASSERT_EQ(li1.start_column_num, 2); //not 3
    ASSERT_EQ(li1.end_line_num, 0);
    ASSERT_EQ(li1.end_column_num, 2); //not 3
    ASSERT_EQ(li1.start_line_begin, 0);

    auto s2 = make<String_Script>(make_string(""),
        make_string("abc\ndef\n"));
    Token t2;
    t2.first = 5;
    t2.last = 6;
    Location l2(*s2, t2);
    Location::Line_Info li2 = l2.line_info();

    ASSERT_EQ(li2.start_line_num, 1);
    ASSERT_EQ(li2.start_column_num, 1);
    ASSERT_EQ(li2.end_line_num, 1);
    ASSERT_EQ(li2.end_column_num, 2);
    ASSERT_EQ(li2.start_line_begin, 4);
}
