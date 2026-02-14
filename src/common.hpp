#pragma once

#include <math.h>
#include <string>
#include <vector>

///  A timestamps in seconds
typedef double timestamp_t;

///  Compare two timestamps
///  They are identical if they are within 1/22050 of a second
inline bool equals(timestamp_t a, timestamp_t b) { return std::fabs(a - b) < 1.0 / 22050; }

/// Convert frame number and fps to tick count (60 ticks per second)
inline size_t ticks_from_frame(size_t n, double fps) { return n / fps * 60 + .5; }

/// Split a string by delimiter
inline std::vector<std::string> split(const std::string &s, const std::string &delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

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
inline bool bool_from(const std::string &v)
{
    if (v == "true")
        return true;
    return false;
}

///  Delete files matching pattern
void delete_files_of_pattern(const std::string &pattern);

void test_simplesprintf();
