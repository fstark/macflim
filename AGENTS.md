## Formatting

Before committing, use ``cd src && make format`` to run clang-format on all .cpp and .hpp files in the src directory. This will ensure consistent code style across the codebase.

## Testing

Test the code using ``cd src && make unit_tests && ./unit_tests``. This will compile and run the unit tests defined in the codebase, which use the doctest framework.

## class naming:

Classes and structs should use snake_case naming (e.g., `encoding_profile`, ...)

## namespaces

All function and variables should be in ``namespace macflim``. Use a single `namespace macflim { ... }` block in each .cpp/.hpp file to contain all definitions, rather than multiple nested namespaces or using `using namespace`.

## logging strategy:

1. Stream selection by purpose:

std::cout — Actual program output meant for users/scripts to consume (results, data files being processed)
std::cerr — Error messages and warnings (things that went wrong)
std::clog — Diagnostic/progress messages behind verbose flags (encoding progress, statistics, debug info)

2. Use std::format() everywhere:

Replace all printf/fprintf with std::format() + stream output
Convert << chains to std::format() for consistency and readability
Type-safe, modern C++20, already in use in the codebase
