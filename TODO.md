# TODO — Codebase Improvements

## 1. Move massive header implementations into `.cpp` files

`flimcompressor.hpp` (799 lines), `compressor.hpp` (~350 lines), `flimencoder.hpp` (~280 lines), and `profile.hpp` (412 lines) contain ~1,840 lines of non-template implementation code in headers. This means any change to the compression logic recompiles *everything*. Several of these also have `using namespace std;` at file scope in a header, which pollutes the namespace of every includer. Moving the implementations into `.cpp` files would dramatically improve build times and make the codebase easier to navigate — each header becomes a concise interface description.

## ~~2. Pass the `profile` object instead of 15 individual parameters~~ ✅ COMPLETED

~~The `compress()` method in `flimcompressor.hpp` takes 15 separate parameters — stability, byterate, group, filters, watermark, codecs, dither, bars, anchor_x, anchor_y, error_algorithm, error_bleed, error_bidi, initial_mode, loop — all of which already live inside the `profile` object that the caller has on hand. The call site in `flimencoder.hpp` is a wall of `profile_.xxx()` getters. Simply passing `const profile&` would:~~
- ~~Shrink the signature from 15 params to 1~~
- ~~Make adding new encoding options trivial (no signature changes)~~
- ~~Eliminate the tight coupling between the caller's knowledge of profile internals and the compressor~~

**Actual change:** +33/-46 lines. Signature changed from 15 parameters to `compress(const encoding_profile &profile, const std::string &watermark, ...)`. Architectural improvement: profile.hpp no longer depends on flimcompressor.hpp (eliminated circular dependency). Profile now stores codec specifications as strings; compressor parses them on demand. Created initial_frame_mode.hpp to share enum definition without circular includes.

## 3. ~~Remove dead code~~ ✅ COMPLETED

~~There's a significant amount of accumulated dead code:~~

| Location | What | Status |
|---|---|---|
| ~~`flimcompressor.hpp`~~ | ~~~300 lines in `#if 0` blocks (old compress implementation)~~ | ✅ Removed ~250 lines of OLD_VERSION code (claim was accurate for OLD code) |
| ~~`bitmap.hpp`~~ | ~~Entire 310-line file — old duplicate of `framebuffer.hpp`, not included anywhere~~ | ❌ **Incorrect claim** — file is actively used by multiple files |
| ~~`reader.cpp`~~ | ~~Completely empty file (0 lines), still compiled and linked~~ | ✅ Removed file and deleted from Makefile |
| ~~`imgcompress.cpp`~~ | ~~~150 lines in `#if 0` with function declarations in the `.hpp` that reference them~~ | ✅ Removed 188 lines + orphaned declarations (packzeroes function saved to IDEAS.md) |
| ~~`filesystem_reader.cpp`~~ | ~~Large commented-out `raw_sound()` method~~ | ✅ Removed 40 lines of commented code |
| ~~`reader.hpp`~~ | ~~Empty `frame_accumulator` class and commented-out coroutine block~~ | ✅ Removed ~18 lines |

**Actual removal:** ~496 lines of dead code removed. The `bitmap.hpp` file is NOT dead code as it's actively included and used.

## 4. Remove stray debug `printf`s from production paths

`flim.cpp` prints `HEADER: ...` and `-> : ...` to stdout on every file write, which pollutes program output. There are also scattered `printf`/`fprintf` debug statements throughout `compressor.hpp` and `flimcompressor.hpp`. Meanwhile, `flimmaker.cpp` already has a `verbose` flag. The fix is straightforward: gate all diagnostic output behind the existing verbose flag (or just delete the leftover debug prints).

## 5. ~~Make `profile.hpp` data-driven instead of copy-paste~~

~~The `profile::from_string()` method in `profile.hpp` contains ~10 nearly identical blocks of ~25 lines each, one per Mac model profile (128k, 512k, Plus, SE, Classic, etc.), differing only in parameter values. This could be replaced with a small table of structs, shrinking ~220 lines of repetitive code down to ~30, and making adding a new profile a single line instead of a copy-paste-modify exercise.~~

## 6. Replace `throw "string literal"` with proper exceptions

Throughout the codebase (`flimmaker.cpp`, `ffmpeg_reader.cpp`, `flim.cpp`, `writer.cpp`), errors are raised by throwing raw C string literals like `throw "Cannot open file"`. This loses type safety — the only way to catch them is `catch (const char*)` or `catch (...)`, and you get no stack trace or structured error info. The top-level `catch` in `main()` is a bare `catch (...)` that can't even print what went wrong. Replacing these with `std::runtime_error` (or a small custom exception hierarchy) would let the top-level handler print meaningful error messages and allow callers to catch specific failure modes.

## 7. Add RAII wrappers for FFmpeg resources

`ffmpeg_reader.cpp` manually manages raw FFmpeg pointers (`AVFormatContext*`, `AVCodecContext*`, `AVFrame*`, `AVPacket*`) with explicit cleanup in `catch` blocks and a `goto done_decoding` label. If any allocation between open and close throws, resources leak. The class-based `ffmpeg_writer` in `writer.cpp` is slightly better (destructor-based) but still uses raw pointers. Wrapping each FFmpeg type in a small RAII helper (e.g. `unique_ptr` with a custom deleter) would eliminate the manual cleanup, remove the `goto`, and make the code exception-safe by construction.

## 8. Extract argument parsing from the ~500-line `main()`

`flimmaker.cpp`'s `main()` function is roughly 500 lines long. The first ~250 lines are a chain of `strcmp`/`argv` pointer arithmetic for CLI parsing, followed by the actual encoding orchestration. Extracting the argument parsing into its own function (returning a config struct) — or using a lightweight CLI library — would cut `main()` in half and cleanly separate "what did the user ask for" from "do the work". It would also make it easier to add new flags without scrolling through hundreds of lines.

## 9. Replace `system()` calls with safer alternatives

`writer.cpp`'s `gif_writer` calls `system(buffer)` with a `sprintf`-built command to invoke ImageMagick `convert` — and it does this *inside a destructor*, which is dangerous (exceptions from `system()` during stack unwinding = undefined behavior). It also uses a hard-coded `/tmp/gif-*.pgm` path, which is fragile and not safe for concurrent runs. Similarly, `flimmaker.cpp` shells out to `yt-dlp`/`youtube-dl` via `system()` with user-provided URLs interpolated via `sprintf`, which is a command injection risk. These should either use `fork`/`exec` (or `popen`) with proper argument lists, or be replaced with library calls where possible.

## 10. Replace hard-coded magic numbers with named constants

The codebase has many unexplained numeric literals scattered across files:
- `512` and `342` (Mac screen dimensions) appear in `ffmpeg_reader.cpp`, `filesystem_reader.cpp`, and elsewhere, but are never defined as constants
- `735` in `writer.cpp` is the number of audio samples per video frame (22257 Hz ÷ 30.27 fps) but reads as a mystery number
- `1500` in `writer.cpp` is a pts increment tied to the time base — unclear without a comment
- `370` in `flimcompressor.hpp` is the sound frame payload size — domain knowledge baked into a literal
- `15` in `grayscale.cpp` is a hard-coded PGM header size that will break on any PGM file with a different header layout

Defining these as named constants (e.g. `constexpr size_t kMacScreenW = 512`) in a shared header would make the code self-documenting and ensure consistency when the same value is used in multiple places.
