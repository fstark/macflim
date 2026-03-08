#include "common.hpp"

#include <assert.h>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace macflim
{

/** Replaces the format by the value v
 * Format can use %d and %0nd (%01d, %02d, etc...)
 * Result is similar to sprintf
 */
std::string simplesprintf(const std::string &format, int v)
{
    std::ostringstream result;
    std::string::const_iterator it = format.begin();

    while (it != format.end())
    {
        if (*it == '%' && (it + 1) != format.end())
        {
            ++it;
            if (*it == 'd')
            {
                result << v;
            }
            else if (std::isdigit(*it))
            {
                ++it;
                int width = 0;

                while (it != format.end() && std::isdigit(*it))
                {
                    width = width * 10 + (*it - '0');
                    ++it;
                }

                if (it != format.end() && *it == 'd')
                {
                    result << std::setw(width) << std::setfill('0') << v;
                }
                else
                {
                    throw std::runtime_error("Invalid format string %: expected 'd'");
                }
            }
            else
            {
                throw std::runtime_error("Invalid format string: % must be followed by digit or 'd'");
            }
        }
        else
        {
            result << *it;
        }
        ++it;
    }

    return result.str();
}

void test_simplesprintf()
{
    // Test cases
    std::string result;

    // Test %d
    result = simplesprintf("%d", 42);
    assert(result == "42");

    // Test %1d
    result = simplesprintf("%1d", 42);
    assert(result == "42");

    // Test %03d
    result = simplesprintf("%03d", 42);
    assert(result == "042");

    // Test %05d
    result = simplesprintf("%05d", 42);
    assert(result == "00042");

    // Test no format
    result = simplesprintf("xxx", 42);
    assert(result == "xxx");

    // Test double format
    result = simplesprintf("%d%d", 42);
    assert(result == "4242");

    // Test extra chars
    result = simplesprintf("v=%d!", 42);
    assert(result == "v=42!");

    // Test invalid format
    try
    {
        result = simplesprintf("%s", 42);
        assert(false); // Should not reach here
    }
    catch (const std::runtime_error &e)
    {
        assert(true); // Expected exception
    }

    // Test mixed text and format
    result = simplesprintf("Value: %d", 42);
    assert(result == "Value: 42");

    result = simplesprintf("Value: %03d", 42);
    assert(result == "Value: 042");
}

std::vector<std::string> split(std::string_view s, std::string_view delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string_view::npos)
    {
        token = std::string(s.substr(pos_start, pos_end - pos_start));
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(std::string(s.substr(pos_start)));
    return res;
}

bool bool_from(std::string_view v)
{
    if (v == "true")
        return true;
    return false;
}

bool ends_with(std::string_view value, std::string_view ending)
{
    if (ending.size() > value.size())
        return false;

    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

static int num_from_string(const char **s)
{
    int n = 0;
    while (**s >= '0' && **s <= '9')
    {
        n = n * 10 + (**s - '0');
        (*s)++;
    }
    return n;
}

timestamp_t seconds_from_string(std::string_view s)
{
    // Convert to temporary C string for pointer arithmetic
    std::string temp(s);
    const char *str = temp.c_str();

    double d = 0;
    for (;;)
    {
        if (*str >= '0' && *str <= '9')
            d = d * 60 + num_from_string(&str);
        if (*str != ':')
            break;
        str++;
    }
    if (!*str)
        return d;
    if (*str == '.')
    {
        double f = 1;
        str++;
        while (*str >= '0' && *str <= '9')
        {
            f /= 10;
            d += f * (*str++ - '0');
        }
    }
    return d;
}

void test_seconds_from_string()
{
    assert(seconds_from_string("42") == 42);
    assert(seconds_from_string("05:31") == 331);
    assert(seconds_from_string("2:4") == 124);
    assert(seconds_from_string("02:04.470") == 124.47);
    assert(seconds_from_string("1230.2") == 1230.2);
    assert(seconds_from_string("0001:1:1:3.1toto") == 219663.1);
}

void delete_files_of_pattern(const std::string &pattern)
{
    int i = 0;
    std::string filepath;
    std::clog << std::format("Deleting files of pattern [{}] ...", pattern) << std::flush;
    do
    {
        i++;
        filepath = simplesprintf(pattern, i);
    } while (!remove(filepath.c_str()));
    std::clog << std::format("{} files deleted\n", i);
}

} // namespace macflim
