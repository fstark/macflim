#pragma once

#include "errors.hpp"

#include <string_view>

namespace macflim
{

/// Simple iterator over argc/argv that safely consumes arguments.
class arg_iterator
{
    int argc_;
    char **argv_;

  public:
    arg_iterator(int argc, char **argv) : argc_{argc}, argv_{argv} {}

    [[nodiscard]] bool has_next() const
    {
        return argc_ > 0;
    }

    /// Return current arg and advance. Throws if exhausted.
    std::string_view next()
    {
        if (argc_ <= 0)
            throw flim_error("Expected argument but reached end of command line");
        std::string_view result = *argv_;
        argc_--;
        argv_++;
        return result;
    }

    /// Return current arg without advancing.
    [[nodiscard]] std::string_view peek() const
    {
        if (argc_ <= 0)
            throw flim_error("Expected argument but reached end of command line");
        return *argv_;
    }

    /// Consume and return the next argument value (the one after a flag).
    std::string_view next_value()
    {
        if (argc_ <= 0)
            throw flim_error("Expected value after flag but reached end of command line");
        return next();
    }

    /// Consume next arg if it exists and doesn't start with '-', otherwise return the default.
    std::string_view optional_value(std::string_view default_value)
    {
        if (argc_ <= 0 || (*argv_)[0] == '-')
            return default_value;
        return next();
    }
};

} // namespace macflim
