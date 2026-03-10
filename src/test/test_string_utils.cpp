#include "../common.hpp"
#include "../doctest.h"

namespace macflim
{

TEST_CASE("split - single delimiter")
{
    auto result = split("a,b,c", ",");
    REQUIRE(result.size() == 3);
    CHECK(result[0] == "a");
    CHECK(result[1] == "b");
    CHECK(result[2] == "c");
}

TEST_CASE("split - no delimiter")
{
    auto result = split("hello", ",");
    REQUIRE(result.size() == 1);
    CHECK(result[0] == "hello");
}

TEST_CASE("split - empty string")
{
    auto result = split("", ",");
    REQUIRE(result.size() == 1);
    CHECK(result[0] == "");
}

TEST_CASE("split - delimiter at start")
{
    auto result = split(",a,b", ",");
    REQUIRE(result.size() == 3);
    CHECK(result[0] == "");
    CHECK(result[1] == "a");
    CHECK(result[2] == "b");
}

TEST_CASE("split - delimiter at end")
{
    auto result = split("a,b,", ",");
    REQUIRE(result.size() == 3);
    CHECK(result[0] == "a");
    CHECK(result[1] == "b");
    CHECK(result[2] == "");
}

TEST_CASE("split - consecutive delimiters")
{
    auto result = split("a,,b", ",");
    REQUIRE(result.size() == 3);
    CHECK(result[0] == "a");
    CHECK(result[1] == "");
    CHECK(result[2] == "b");
}

TEST_CASE("split - multi-char delimiter")
{
    auto result = split("a::b::c", "::");
    REQUIRE(result.size() == 3);
    CHECK(result[0] == "a");
    CHECK(result[1] == "b");
    CHECK(result[2] == "c");
}

TEST_CASE("split - space delimiter")
{
    auto result = split("hello world test", " ");
    REQUIRE(result.size() == 3);
    CHECK(result[0] == "hello");
    CHECK(result[1] == "world");
    CHECK(result[2] == "test");
}

TEST_CASE("split - only delimiter")
{
    auto result = split(",", ",");
    REQUIRE(result.size() == 2);
    CHECK(result[0] == "");
    CHECK(result[1] == "");
}

TEST_CASE("bool_from - true string")
{
    CHECK(bool_from("true") == true);
}

TEST_CASE("bool_from - false by default")
{
    CHECK(bool_from("false") == false);
    CHECK(bool_from("") == false);
    CHECK(bool_from("anything") == false);
    CHECK(bool_from("True") == false); // case sensitive
    CHECK(bool_from("TRUE") == false);
    CHECK(bool_from("1") == false);
    CHECK(bool_from("yes") == false);
}

TEST_CASE("ends_with - basic suffix")
{
    CHECK(ends_with("hello.txt", ".txt") == true);
    CHECK(ends_with("test.cpp", ".cpp") == true);
    CHECK(ends_with("file.tar.gz", ".gz") == true);
}

TEST_CASE("ends_with - no match")
{
    CHECK(ends_with("hello.txt", ".cpp") == false);
    CHECK(ends_with("test", ".txt") == false);
    CHECK(ends_with("file.tar.gz", ".tar") == false);
}

TEST_CASE("ends_with - exact match")
{
    CHECK(ends_with("hello", "hello") == true);
    CHECK(ends_with(".txt", ".txt") == true);
}

TEST_CASE("ends_with - suffix longer than string")
{
    CHECK(ends_with("hi", "hello") == false);
    CHECK(ends_with("", "suffix") == false);
}

TEST_CASE("ends_with - empty suffix")
{
    CHECK(ends_with("hello", "") == true);
    CHECK(ends_with("", "") == true);
}

TEST_CASE("ends_with - empty string")
{
    CHECK(ends_with("", "suffix") == false);
    CHECK(ends_with("", "") == true);
}

TEST_CASE("ends_with - case sensitive")
{
    CHECK(ends_with("Hello.TXT", ".txt") == false);
    CHECK(ends_with("file.cpp", ".CPP") == false);
    CHECK(ends_with("test.txt", ".txt") == true);
}

TEST_CASE("ends_with - partial match not at end")
{
    CHECK(ends_with("hello.txt.bak", ".txt") == false);
    CHECK(ends_with("prefix.middle.suffix", ".middle") == false);
}

} // namespace macflim
