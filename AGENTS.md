## Testing

Test the code using ``cd src && make test``

## class naming:

Classes and structs should use snake_case naming (e.g., `encoding_profile`, ...)

## logging strategy:

1. Stream selection by purpose:

std::cout — Actual program output meant for users/scripts to consume (results, data files being processed)
std::cerr — Error messages and warnings (things that went wrong)
std::clog — Diagnostic/progress messages behind verbose flags (encoding progress, statistics, debug info)

2. Use std::format() everywhere:

Replace all printf/fprintf with std::format() + stream output
Convert << chains to std::format() for consistency and readability
Type-safe, modern C++20, already in use in the codebase
