#pragma once

#include <stdexcept>
#include <string>

extern "C"
{
#include <libavutil/error.h>
}

namespace macflim
{

// Base exception class for all MacFlim errors
class flim_error : public std::runtime_error
{
  public:
    explicit flim_error(const std::string &message) : std::runtime_error(message) {}
};

// FFmpeg-specific errors with error code translation
class ffmpeg_error : public flim_error
{
  private:
    int error_code_;
    std::string full_message_;

  public:
    ffmpeg_error(const std::string &message, int error_code) : flim_error(message), error_code_(error_code)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(error_code, errbuf, sizeof(errbuf));
        full_message_ = message + ": " + errbuf;
    }

    ffmpeg_error(const std::string &message, int error_code, const std::string &path)
        : flim_error(message), error_code_(error_code)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(error_code, errbuf, sizeof(errbuf));
        full_message_ = message + " '" + path + "': " + errbuf;
    }

    const char *what() const noexcept override
    {
        return full_message_.c_str();
    }

    [[nodiscard]] int error_code() const noexcept
    {
        return error_code_;
    }
};

// I/O errors with file path context
class io_error : public flim_error
{
  private:
    std::string path_;
    std::string full_message_;

  public:
    io_error(const std::string &message, const std::string &path) : flim_error(message), path_(path)
    {
        full_message_ = message + ": " + path;
    }

    const char *what() const noexcept override
    {
        return full_message_.c_str();
    }

    [[nodiscard]] const std::string &path() const noexcept
    {
        return path_;
    }
};

// Configuration/validation errors with option context
class config_error : public flim_error
{
  private:
    std::string option_;
    std::string full_message_;

  public:
    config_error(const std::string &message, const std::string &option) : flim_error(message), option_(option)
    {
        full_message_ = message + ": " + option;
    }

    const char *what() const noexcept override
    {
        return full_message_.c_str();
    }

    const std::string &option() const noexcept
    {
        return option_;
    }
};

// Early exit (non-error): mode switch, help display, version, etc.
class early_exit : public std::exception
{
  private:
    int exit_code_;
    std::string message_; // Optional message to display on stdout

  public:
    // Exit with code, no output
    explicit early_exit(int code) : exit_code_(code), message_() {}

    // Exit with code and message to stdout (--help, --version)
    early_exit(int code, std::string message) : exit_code_(code), message_(std::move(message)) {}

    [[nodiscard]] int exit_code() const noexcept
    {
        return exit_code_;
    }

    [[nodiscard]] bool has_message() const noexcept
    {
        return !message_.empty();
    }

    const char *what() const noexcept override
    {
        return message_.c_str();
    }
};

} // namespace macflim
