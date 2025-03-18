#include "common.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <assert.h>

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
    try {
        result = simplesprintf("%s", 42);
        assert(false); // Should not reach here
    } catch (const std::runtime_error& e) {
        assert(true); // Expected exception
    }

    // Test mixed text and format
    result = simplesprintf("Value: %d", 42);
    assert(result == "Value: 42");

    result = simplesprintf("Value: %03d", 42);
    assert(result == "Value: 042");
}

void delete_files_of_pattern(const std::string &pattern)
{
    int i = 0;
    std::string filepath;
    std::clog << "Deleting files of pattern [" << pattern << "] ..." << std::flush;
    do
    {
        i++;
        filepath = simplesprintf(pattern,i);
    } while (!remove(filepath.c_str()));
    std::clog << i << " files deleted\n";
}
