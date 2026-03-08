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

**Status:** The top 6 longest functions have been successfully refactored into smaller helper functions.

Functions should target ≤20 lines, with a hard limit of 30 lines (debug statements excluded; `test_*` functions exempt). The remaining 22 functions that exceed the 30-line limit should be broken down into smaller, more focused helper functions:

1. **pushFrame** - [writer.cpp](src/writer.cpp#L87-L183) - **97 lines**
2. **flimutil_main** - [flimutil.cpp](src/flimutil.cpp#L239-L325) - **87 lines**
3. **usage** - [cmdline.cpp](src/cmdline.cpp#L17-L88) - **72 lines**
4. **flimcompressor::compress** - [flimcompressor.cpp](src/flimcompressor.cpp#L11-L81) - **71 lines**
5. **dump_frame_data** - [flimutil.cpp](src/flimutil.cpp#L91-L161) - **71 lines**
6. **compressor_helper::add** - [compressor_helper.cpp](src/compressor_helper.cpp#L11-L80) - **70 lines**
7. **packbits** - [imgcompress.cpp](src/imgcompress.cpp#L10-L74) - **65 lines**
8. **error_diffusion** - [grayscale.cpp](src/grayscale.cpp#L683-L740) - **58 lines**
9. **old_quantize** - [grayscale.cpp](src/grayscale.cpp#L625-L678) - **54 lines**
10. **next (ffmpeg_reader)** - [ffmpeg_reader.cpp](src/ffmpeg_reader.cpp#L234-L282) - **49 lines**
11. **simplesprintf** - [common.cpp](src/common.cpp#L12-L57) - **46 lines**
12. **filter (with filter_type)** - [grayscale.cpp](src/grayscale.cpp#L451-L488) - **38 lines**
13. **~ffmpeg_writer destructor** - [writer.cpp](src/writer.cpp#L398-L432) - **35 lines**
14. **dump_frame** - [flimutil.cpp](src/flimutil.cpp#L163-L196) - **34 lines**
15. **receive_video_frame** - [ffmpeg_reader.cpp](src/ffmpeg_reader.cpp#L109-L136) - **28 lines**
16. **init_video_context** - [ffmpeg_reader.cpp](src/ffmpeg_reader.cpp#L138-L165) - **28 lines**
17. **blur5** - [grayscale.cpp](src/grayscale.cpp#L123-L149) - **27 lines**
18. **debug_filter** - [grayscale.cpp](src/grayscale.cpp#L237-L263) - **27 lines**
19. **blur3** - [grayscale.cpp](src/grayscale.cpp#L94-L120) - **27 lines**
20. **read_grayscale** - [grayscale.cpp](src/grayscale.cpp#L525-L550) - **26 lines**
21. **sharpen** - [grayscale.cpp](src/grayscale.cpp#L72-L92) - **21 lines**
22. **write_grayscale** - [grayscale.cpp](src/grayscale.cpp#L555-L573) - **21 lines**

Items 15-22 are near the 30-line threshold and may be acceptable as-is depending on complexity.

