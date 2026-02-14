# TODO — Codebase Improvements

## 1. Extract argument parsing from the ~500-line `main()`

`flimmaker.cpp`'s `main()` function is roughly 500 lines long. The first ~250 lines are a chain of `strcmp`/`argv` pointer arithmetic for CLI parsing, followed by the actual encoding orchestration. Extracting the argument parsing into its own function (returning a config struct) — or using a lightweight CLI library — would cut `main()` in half and cleanly separate "what did the user ask for" from "do the work". It would also make it easier to add new flags without scrolling through hundreds of lines.

## 2. Replace `system()` calls with safer alternatives

`writer.cpp`'s `gif_writer` calls `system(buffer)` with a `sprintf`-built command to invoke ImageMagick `convert` — and it does this *inside a destructor*, which is dangerous (exceptions from `system()` during stack unwinding = undefined behavior). It also uses a hard-coded `/tmp/gif-*.pgm` path, which is fragile and not safe for concurrent runs. Similarly, `flimmaker.cpp` shells out to `yt-dlp`/`youtube-dl` via `system()` with user-provided URLs interpolated via `sprintf`, which is a command injection risk. These should either use `fork`/`exec` (or `popen`) with proper argument lists, or be replaced with library calls where possible.

## 3. Replace hard-coded magic numbers with named constants

The codebase has many unexplained numeric literals scattered across files:
- `512` and `342` (Mac screen dimensions) appear in `ffmpeg_reader.cpp`, `filesystem_reader.cpp`, and elsewhere, but are never defined as constants
- `735` in `writer.cpp` is the number of audio samples per video frame (22257 Hz ÷ 30.27 fps) but reads as a mystery number
- `1500` in `writer.cpp` is a pts increment tied to the time base — unclear without a comment
- `370` in `flimcompressor.hpp` is the sound frame payload size — domain knowledge baked into a literal
- `15` in `grayscale.cpp` is a hard-coded PGM header size that will break on any PGM file with a different header layout

Defining these as named constants (e.g. `constexpr size_t kMacScreenW = 512`) in a shared header would make the code self-documenting and ensure consistency when the same value is used in multiple places.

