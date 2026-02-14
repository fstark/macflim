# TODO — Codebase Improvements

## 1. Move massive header implementations into `.cpp` files

`flimcompressor.hpp` (799 lines), `compressor.hpp` (~350 lines), `flimencoder.hpp` (~280 lines), and `profile.hpp` (412 lines) contain ~1,840 lines of non-template implementation code in headers. This means any change to the compression logic recompiles *everything*. Several of these also have `using namespace std;` at file scope in a header, which pollutes the namespace of every includer. Moving the implementations into `.cpp` files would dramatically improve build times and make the codebase easier to navigate — each header becomes a concise interface description.

## 2. Extract argument parsing from the ~500-line `main()`

`flimmaker.cpp`'s `main()` function is roughly 500 lines long. The first ~250 lines are a chain of `strcmp`/`argv` pointer arithmetic for CLI parsing, followed by the actual encoding orchestration. Extracting the argument parsing into its own function (returning a config struct) — or using a lightweight CLI library — would cut `main()` in half and cleanly separate "what did the user ask for" from "do the work". It would also make it easier to add new flags without scrolling through hundreds of lines.

## 3. Replace `system()` calls with safer alternatives

`writer.cpp`'s `gif_writer` calls `system(buffer)` with a `sprintf`-built command to invoke ImageMagick `convert` — and it does this *inside a destructor*, which is dangerous (exceptions from `system()` during stack unwinding = undefined behavior). It also uses a hard-coded `/tmp/gif-*.pgm` path, which is fragile and not safe for concurrent runs. Similarly, `flimmaker.cpp` shells out to `yt-dlp`/`youtube-dl` via `system()` with user-provided URLs interpolated via `sprintf`, which is a command injection risk. These should either use `fork`/`exec` (or `popen`) with proper argument lists, or be replaced with library calls where possible.

## 4. Replace hard-coded magic numbers with named constants

The codebase has many unexplained numeric literals scattered across files:
- `512` and `342` (Mac screen dimensions) appear in `ffmpeg_reader.cpp`, `filesystem_reader.cpp`, and elsewhere, but are never defined as constants
- `735` in `writer.cpp` is the number of audio samples per video frame (22257 Hz ÷ 30.27 fps) but reads as a mystery number
- `1500` in `writer.cpp` is a pts increment tied to the time base — unclear without a comment
- `370` in `flimcompressor.hpp` is the sound frame payload size — domain knowledge baked into a literal
- `15` in `grayscale.cpp` is a hard-coded PGM header size that will break on any PGM file with a different header layout

Defining these as named constants (e.g. `constexpr size_t kMacScreenW = 512`) in a shared header would make the code self-documenting and ensure consistency when the same value is used in multiple places.

## 5. `encoding_result.hpp` and `compressor_helper.hpp`: Fix double-penalty bug

In `CompressorHelper::add` (line 113), the budget is already scaled by `codec.penality` before being passed to `EncodingResult`, whose constructor (line 27) *also* scales by `codec_.penality` — applying the factor twice. Remove one of these multiplications to make penalty application explicit in exactly one place.

## 6. `compressor_helper.hpp`: Simplify tick-grouping double loop and fix shadowed `i`

The outer `for (size_t i = ...)` at line 87 and inner `for (size_t i = ...)` at line 91 shadow the same variable — a latent bug. The `group_` flag makes the outer loop run either once or N times and the inner loop does the inverse, so they always process the same total ticks. Replace with a single loop iterating over sub-frames (count = `ticks / local_ticks`), each gathering `local_ticks` audio frames via `std::copy_n`. This eliminates the shadowing, removes a nesting level, and makes the grouping semantics self-documenting.
