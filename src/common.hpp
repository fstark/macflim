#pragma once

#include <math.h>

///  A timestamps in seconds
typedef double timestamp_t;

///  Compare two timestamps
///  They are identical if they are within 1/22050 of a second
inline bool equals(timestamp_t a, timestamp_t b) { return std::fabs(a - b) < 1.0 / 22050; }

/// Alternate implementation of std::popcount, to support non compliant C++20 compilers (MacOS 10.15)
inline int mypopcount( unsigned n )
{
    int count = 0;
    while (n) {
        count ++;
        n &= n-1;
    }
    return count;
}

/// Boolean from string
inline bool bool_from( const std::string &v )
{
    if (v=="true")
        return true;
    return false;
}

