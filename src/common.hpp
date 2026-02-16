#pragma once

#include <math.h>
#include <string>
#include <vector>
#include <assert.h>

namespace macflim
{

///  A timestamps in seconds
typedef double timestamp_t;

///  Compare two timestamps
///  They are identical if they are within 1/22050 of a second
inline bool equals(timestamp_t a, timestamp_t b) { return std::fabs(a - b) < 1.0 / 22050; }

/// Convert frame number and fps to tick count (60 ticks per second)
inline size_t ticks_from_frame(size_t n, double fps) { return n / fps * 60 + .5; }

/// Split a string by delimiter
std::vector<std::string> split(const std::string &s, const std::string &delimiter);

/// Alternate implementation of std::popcount, to support non compliant C++20 compilers (MacOS 10.15)
inline int mypopcount(unsigned n)
{
    int count = 0;
    while (n)
    {
        count++;
        n &= n - 1;
    }
    return count;
}

/// Boolean from string
bool bool_from(const std::string &v);

/// Check if string ends with suffix
bool ends_with(std::string const &value, std::string const &ending);

/// Converts a timestamp into a second count
/// 42 => 42
/// 05:31 => 331
/// 2:4 => 124
/// 02:04.470 => 124.47
/// 1230.2 => 1230.2
/// 0001:1:1:3.1toto => 219663.1
timestamp_t seconds_from_string(const char *s);

///  Delete files matching pattern
void delete_files_of_pattern(const std::string &pattern);

void test_simplesprintf();

/// Test seconds_from_string function
void test_seconds_from_string();

} // namespace macflim
