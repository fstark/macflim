#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#include <iostream>

// Custom main to suppress std::clog during tests
int main(int argc, char **argv)
{
    // Redirect std::clog to null buffer to suppress diagnostic output during tests
    std::streambuf *original_clog = std::clog.rdbuf();
    std::clog.rdbuf(nullptr);

    // Also redirect std::cerr temporarily (qhistogram uses it for table rows)
    std::streambuf *original_cerr = std::cerr.rdbuf();
    std::cerr.rdbuf(nullptr);

    doctest::Context context;
    context.applyCommandLine(argc, argv);

    int res = context.run();

    // Restore original buffers
    std::clog.rdbuf(original_clog);
    std::cerr.rdbuf(original_cerr);

    return res;
}
