#include "doctest.h"

#include "common.hpp"

using namespace macflim;

// --- simplesprintf tests (migrated from common.cpp) ---

TEST_CASE("simplesprintf: basic %d")
{
    CHECK(simplesprintf("%d", 42) == "42");
}

TEST_CASE("simplesprintf: %1d width specifier")
{
    CHECK(simplesprintf("%1d", 42) == "42");
}

TEST_CASE("simplesprintf: zero-padded %03d")
{
    CHECK(simplesprintf("%03d", 42) == "042");
}

TEST_CASE("simplesprintf: zero-padded %05d")
{
    CHECK(simplesprintf("%05d", 42) == "00042");
}

TEST_CASE("simplesprintf: no format specifier")
{
    CHECK(simplesprintf("xxx", 42) == "xxx");
}

TEST_CASE("simplesprintf: double format")
{
    CHECK(simplesprintf("%d%d", 42) == "4242");
}

TEST_CASE("simplesprintf: extra chars around format")
{
    CHECK(simplesprintf("v=%d!", 42) == "v=42!");
}

TEST_CASE("simplesprintf: invalid format throws")
{
    CHECK_THROWS(static_cast<void>(simplesprintf("%s", 42)));
}

TEST_CASE("simplesprintf: mixed text and format")
{
    CHECK(simplesprintf("Value: %d", 42) == "Value: 42");
    CHECK(simplesprintf("Value: %03d", 42) == "Value: 042");
}

// --- seconds_from_string tests (migrated from common.cpp) ---

TEST_CASE("seconds_from_string: integer seconds")
{
    CHECK(seconds_from_string("42") == 42);
}

TEST_CASE("seconds_from_string: mm:ss")
{
    CHECK(seconds_from_string("05:31") == 331);
}

TEST_CASE("seconds_from_string: short mm:ss")
{
    CHECK(seconds_from_string("2:4") == 124);
}

TEST_CASE("seconds_from_string: mm:ss.fff")
{
    CHECK(seconds_from_string("02:04.470") == 124.47);
}

TEST_CASE("seconds_from_string: seconds with decimal")
{
    CHECK(seconds_from_string("1230.2") == 1230.2);
}

TEST_CASE("seconds_from_string: hh:mm:ss:ff with trailing text")
{
    CHECK(seconds_from_string("0001:1:1:3.1toto") == 219663.1);
}
