#include "../arg_iterator.hpp"
#include "../doctest.h"
#include "../errors.hpp"

namespace macflim
{

TEST_CASE("arg_iterator - basic next")
{
    char arg0[] = "program";
    char arg1[] = "arg1";
    char arg2[] = "arg2";
    char *argv[] = {arg0, arg1, arg2};
    arg_iterator it(3, argv);

    CHECK(it.has_next());
    CHECK(it.next() == "program");
    CHECK(it.has_next());
    CHECK(it.next() == "arg1");
    CHECK(it.has_next());
    CHECK(it.next() == "arg2");
    CHECK_FALSE(it.has_next());
}

TEST_CASE("arg_iterator - next when exhausted throws")
{
    char arg0[] = "program";
    char *argv[] = {arg0};
    arg_iterator it(1, argv);

    it.next(); // consume the one arg
    CHECK_FALSE(it.has_next());
    CHECK_THROWS_AS(it.next(), flim_error);
}

TEST_CASE("arg_iterator - peek without consuming")
{
    char arg0[] = "program";
    char arg1[] = "arg1";
    char *argv[] = {arg0, arg1};
    arg_iterator it(2, argv);

    CHECK(it.peek() == "program");
    CHECK(it.has_next());
    CHECK(it.peek() == "program"); // Still the same
    CHECK(it.next() == "program"); // Now consume it
    CHECK(it.peek() == "arg1");
    CHECK(it.next() == "arg1");
}

TEST_CASE("arg_iterator - peek when exhausted throws")
{
    char arg0[] = "program";
    char *argv[] = {arg0};
    arg_iterator it(1, argv);

    it.next();
    CHECK_THROWS_AS((void)it.peek(), flim_error);
}

TEST_CASE("arg_iterator - next_value")
{
    char arg0[] = "-flag";
    char arg1[] = "value";
    char arg2[] = "next";
    char *argv[] = {arg0, arg1, arg2};
    arg_iterator it(3, argv);

    CHECK(it.next() == "-flag");
    CHECK(it.next_value() == "value");
    CHECK(it.next() == "next");
}

TEST_CASE("arg_iterator - next_value when exhausted throws")
{
    char arg0[] = "-flag";
    char *argv[] = {arg0};
    arg_iterator it(1, argv);

    it.next();
    CHECK_THROWS_AS(it.next_value(), flim_error);
}

TEST_CASE("arg_iterator - optional_value with value present")
{
    char arg0[] = "-flag";
    char arg1[] = "value";
    char arg2[] = "next";
    char *argv[] = {arg0, arg1, arg2};
    arg_iterator it(3, argv);

    it.next(); // consume -flag
    CHECK(it.optional_value("default") == "value");
    CHECK(it.has_next());
    CHECK(it.next() == "next");
}

TEST_CASE("arg_iterator - optional_value returns default when exhausted")
{
    char arg0[] = "-flag";
    char *argv[] = {arg0};
    arg_iterator it(1, argv);

    it.next(); // consume -flag
    CHECK(it.optional_value("default") == "default");
    CHECK_FALSE(it.has_next());
}

TEST_CASE("arg_iterator - optional_value returns default when next starts with dash")
{
    char arg0[] = "-flag1";
    char arg1[] = "-flag2";
    char *argv[] = {arg0, arg1};
    arg_iterator it(2, argv);

    it.next(); // consume -flag1
    CHECK(it.optional_value("default") == "default");
    CHECK(it.has_next());
    CHECK(it.next() == "-flag2"); // Not consumed by optional_value
}

TEST_CASE("arg_iterator - optional_value consumes non-dash argument")
{
    char arg0[] = "-flag";
    char arg1[] = "value";
    char *argv[] = {arg0, arg1};
    arg_iterator it(2, argv);

    it.next(); // consume -flag
    CHECK(it.optional_value("default") == "value");
    CHECK_FALSE(it.has_next()); // value was consumed
}

TEST_CASE("arg_iterator - empty iterator")
{
    char *argv[] = {};
    arg_iterator it(0, argv);

    CHECK_FALSE(it.has_next());
    CHECK_THROWS_AS(it.next(), flim_error);
    CHECK_THROWS_AS((void)it.peek(), flim_error);
    CHECK_THROWS_AS(it.next_value(), flim_error);
    CHECK(it.optional_value("default") == "default");
}

TEST_CASE("arg_iterator - single argument")
{
    char arg0[] = "program";
    char *argv[] = {arg0};
    arg_iterator it(1, argv);

    CHECK(it.has_next());
    CHECK(it.next() == "program");
    CHECK_FALSE(it.has_next());
}

TEST_CASE("arg_iterator - mix of operations")
{
    char arg0[] = "program";
    char arg1[] = "-input";
    char arg2[] = "file.txt";
    char arg3[] = "-output";
    char arg4[] = "out.txt";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4};
    arg_iterator it(5, argv);

    CHECK(it.next() == "program");
    CHECK(it.peek() == "-input");
    CHECK(it.next() == "-input");
    CHECK(it.next_value() == "file.txt");
    CHECK(it.next() == "-output");
    CHECK(it.optional_value("default") == "out.txt");
    CHECK_FALSE(it.has_next());
}

TEST_CASE("arg_iterator - arguments with special characters")
{
    char arg0[] = "program";
    char arg1[] = "--long-flag";
    char arg2[] = "value with spaces";
    char arg3[] = "-x";
    char *argv[] = {arg0, arg1, arg2, arg3};
    arg_iterator it(4, argv);

    CHECK(it.next() == "program");
    CHECK(it.next() == "--long-flag");
    CHECK(it.next() == "value with spaces");
    CHECK(it.next() == "-x");
}

TEST_CASE("arg_iterator - negative number as value")
{
    char arg0[] = "-threshold";
    char arg1[] = "-5";
    char *argv[] = {arg0, arg1};
    arg_iterator it(2, argv);

    it.next(); // consume -threshold
    // optional_value sees "-5" starts with dash, so returns default
    CHECK(it.optional_value("10") == "10");
    CHECK(it.has_next());
    CHECK(it.next() == "-5");
}

TEST_CASE("arg_iterator - next_value with negative number")
{
    char arg0[] = "-threshold";
    char arg1[] = "-5";
    char *argv[] = {arg0, arg1};
    arg_iterator it(2, argv);

    it.next(); // consume -threshold
    // next_value doesn't check for dash, it just consumes
    CHECK(it.next_value() == "-5");
    CHECK_FALSE(it.has_next());
}

} // namespace macflim
