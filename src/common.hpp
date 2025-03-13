#pragma once

///  A timestamps in seconds
typedef double timestamp_t;

///  Compare two timestamps
///  They are identical if they are within 1/22050 of a second
inline bool equals(timestamp_t a, timestamp_t b) { return std::fabs(a - b) < 1.0 / 22050; }
