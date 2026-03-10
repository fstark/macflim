// Test stubs for symbols normally defined in flimmaker.cpp (main)

#include <string>

namespace macflim
{

bool sDebug = false;
const char *version = "test-version";

const std::string temp_file()
{
    return "/tmp/test_temp_file.tmp";
}

} // namespace macflim
