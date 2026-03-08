#include "common.hpp"

#include <cassert>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace macflim
{

/// Parse a width specifier from a format string (%Nd where N is width).
/// Advances iterator past the complete specifier and returns the width.
static int parse_width_specifier(std::string::const_iterator &it, std::string::const_iterator end)
{
    int width = 0;
    while (it != end && std::isdigit(*it))
    {
        width = width * 10 + (*it - '0');
        ++it;
    }

    if (it != end && *it == 'd')
        return width;

    throw std::runtime_error("Invalid format string %: expected 'd'");
}

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
                int width = parse_width_specifier(it, format.end());
                result << std::setw(width) << std::setfill('0') << v;
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
