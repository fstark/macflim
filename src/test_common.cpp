#include "doctest.h"

#include "common.hpp"

#include <cstdio>
#include <stdexcept>

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

TEST_CASE("simplesprintf: width without 'd' throws")
{
    // Format like %5x should throw - digits but not ending in 'd'
    CHECK_THROWS_AS(simplesprintf("%5x", 42), std::runtime_error);
    CHECK_THROWS_AS(simplesprintf("%10s", 42), std::runtime_error);
    CHECK_THROWS_AS(simplesprintf("%3", 42), std::runtime_error);
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

// --- delete_files_of_pattern tests ---

TEST_CASE("delete_files_of_pattern: deletes numbered files")
{
    // Create test files in /tmp/
    std::string pattern = "/tmp/test_delete_%03d.tmp";

    // Create 5 test files
    for (int i = 1; i <= 5; i++)
    {
        std::string filepath = simplesprintf(pattern, i);
        FILE *f = fopen(filepath.c_str(), "w");
        REQUIRE(f != nullptr);
        fprintf(f, "test file %d\n", i);
        fclose(f);
    }

    // Verify files exist
    for (int i = 1; i <= 5; i++)
    {
        std::string filepath = simplesprintf(pattern, i);
        FILE *f = fopen(filepath.c_str(), "r");
        REQUIRE(f != nullptr);
        fclose(f);
    }

    // Delete them using the pattern
    delete_files_of_pattern(pattern);

    // Verify files are deleted
    for (int i = 1; i <= 5; i++)
    {
        std::string filepath = simplesprintf(pattern, i);
        FILE *f = fopen(filepath.c_str(), "r");
        CHECK(f == nullptr);
    }
}

TEST_CASE("delete_files_of_pattern: handles no files gracefully")
{
    // Use a pattern that won't match any files
    std::string pattern = "/tmp/test_nonexistent_file_%05d.tmp";

    // Should not crash, just delete 0 files
    delete_files_of_pattern(pattern);

    // Nothing to verify - just checking it doesn't crash
}
