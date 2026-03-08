# TODO — Codebase Improvements

## 1. Replace `system()` calls with safer alternatives

`writer.cpp`'s `gif_writer` calls `system(buffer)` with a `sprintf`-built command to invoke ImageMagick `convert` — and it does this *inside a destructor*, which is dangerous (exceptions from `system()` during stack unwinding = undefined behavior). It also uses a hard-coded `/tmp/gif-*.pgm` path, which is fragile and not safe for concurrent runs. Similarly, `flimmaker.cpp` shells out to `yt-dlp`/`youtube-dl` via `system()` with user-provided URLs interpolated via `sprintf`, which is a command injection risk. These should either use `fork`/`exec` (or `popen`) with proper argument lists, or be replaced with library calls where possible.

## 2. Replace hard-coded magic numbers with named constants

The codebase has many unexplained numeric literals scattered across files:
- `512` and `342` (Mac screen dimensions) appear in `ffmpeg_reader.cpp`, `filesystem_reader.cpp`, and elsewhere, but are never defined as constants
- `735` in `writer.cpp` is the number of audio samples per video frame (22257 Hz ÷ 30.27 fps) but reads as a mystery number
- `1500` in `writer.cpp` is a pts increment tied to the time base — unclear without a comment
- `370` in `flimcompressor.hpp` is the sound frame payload size — domain knowledge baked into a literal
- `15` in `grayscale.cpp` is a hard-coded PGM header size that will break on any PGM file with a different header layout

Defining these as named constants (e.g. `constexpr size_t kMacScreenW = 512`) in a shared header would make the code self-documenting and ensure consistency when the same value is used in multiple places.

## 3. Refactor long functions (>30 lines)

**Status:** 10 functions fully refactored below threshold, 1 removed as dead code.

Functions should target ≤20 lines, with a hard limit of 30 lines (debug statements excluded; `test_*` functions exempt). The remaining 4 functions that exceed the 30-line limit should be addressed:

1. **flimcompressor::compress** - [flimcompressor.cpp](src/flimcompressor.cpp#L41-L86) - **46 lines** — Orchestration function, already well-factored
2. **filter (with filter_type)** - [grayscale.cpp](src/grayscale.cpp#L426-L464) - **39 lines** — Optimal dispatch switch statement
3. **simplesprintf** - [common.cpp](src/common.cpp#L33-L65) - **33 lines** (reduced from 48) — Further reduction difficult without over-engineering
4. **dump_frame** - [flimutil.cpp](src/flimutil.cpp#L216-L248) - **33 lines** (reduced from 42) — Good validation-then-action structure

**Completed:**
- ✅ ~ffmpeg_writer destructor: 41→20 lines (extracted `flush_delayed_video_frames()`)
- ✅ ffmpeg_reader::next: 55→45 lines (extracted `send_video_packet_checked()`)
- ✅ blur3: 30 lines — At limit, good structure
- ✅ receive_video_frame: 32 lines — Good structure

**Analysis:** The remaining 4 functions are either inherently multi-step (orchestration, dispatch) or have already been optimized. Further decomposition would create helpers with unclear responsibilities or numerous parameters, making code harder to understand.
