#pragma once

#include <cstdio>
#include <format>
#include <string>
#include <string_view>

namespace macflim
{

/// RAII wrapper for FILE* to ensure automatic closure
/// Replaces raw FILE* usage throughout the codebase
class file_handle
{
  private:
    FILE *file_ = nullptr;
    std::string path_;

  public:
    file_handle() = default;

    file_handle(std::string_view path, std::string_view mode) : path_{path}
    {
        file_ = fopen(path_.c_str(), mode.data());
        if (!file_)
        {
            throw std::runtime_error(std::format("Cannot open file '{}' with mode '{}'", path, mode));
        }
    }

    ~file_handle()
    {
        if (file_)
        {
            fclose(file_);
        }
    }

    // Delete copy operations
    file_handle(const file_handle &) = delete;
    file_handle &operator=(const file_handle &) = delete;

    // Move operations
    file_handle(file_handle &&other) noexcept : file_{other.file_}, path_{std::move(other.path_)}
    {
        other.file_ = nullptr;
    }

    file_handle &operator=(file_handle &&other) noexcept
    {
        if (this != &other)
        {
            if (file_)
            {
                fclose(file_);
            }
            file_ = other.file_;
            path_ = std::move(other.path_);
            other.file_ = nullptr;
        }
        return *this;
    }

    // Get raw FILE* for C API compatibility
    FILE *get() const
    {
        return file_;
    }

    // Conversion operator for easy use with C APIs
    operator FILE *() const
    {
        return file_;
    }

    // Check if file is open
    explicit operator bool() const
    {
        return file_ != nullptr;
    }

    const std::string &path() const
    {
        return path_;
    }
};

} // namespace macflim
