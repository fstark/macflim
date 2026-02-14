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

## 5. `flimcompressor.hpp`: Extract inner classes into separate files

`Ditherer`, `SubtitleBurner`, `CompressorHelper`, `EncodingResult`, and `qhistogram` are self-contained classes nested inside `flimcompressor`. Moving each to its own header (e.g. `ditherer.hpp`, `subtitle_burner.hpp`, `compressor_helper.hpp`) would reduce `flimcompressor.hpp` to ~80 lines of orchestration, improve compile times, and make each piece independently testable.

## 6. `flimcompressor.hpp`: Replace `make_codec` if/else chain with a data-driven registry

`make_codec` is a ~60-line `if/else if` ladder mapping codec name strings to constructors. Replace it with a `static const std::map<std::string, ...>` mapping names to factory lambdas + signature + penalty. This eliminates repetitive branching, makes adding codecs a one-liner, and respects the Open/Closed principle.

## 7. `flimcompressor.hpp`: Fix double-penalty bug and move compression out of `EncodingResult` constructor

In `CompressorHelper::add`, the budget is already scaled by `codec.penality` before being passed to `EncodingResult`, whose constructor *also* scales by `codec_.penality` — applying the factor twice. Refactor `EncodingResult` into a plain data struct populated by a static factory or free function, making the penalty application explicit in exactly one place.

## 8. `flimcompressor.hpp`: Move `split()` to `common.hpp` and delete dead code

The generic string `split()` utility inside `flimcompressor` has nothing to do with compression — move it to `common.hpp`. Also remove the unused public field `progress_` and the unused `#define VERBOSE`, which are dead code.

## 9. `flimcompressor.hpp`: Replace `SubtitleBurner`'s copy-and-erase with an index

`SubtitleBurner` copies the entire subtitle vector, then calls `erase(begin())` — an O(n) operation per subtitle. Replace with a `const` reference (or `std::span`) plus a `size_t` index to advance through the list in O(1), avoiding both the copy and the repeated shuffle.

## 10. `flimcompressor.hpp`: Simplify tick-grouping double loop and fix shadowed `i` in `CompressorHelper::add`

The outer `for (size_t i = ...)` and inner `for (size_t i = ...)` shadow the same variable — a latent bug. The `group_` flag makes the outer loop run either once or N times and the inner loop does the inverse, so they always process the same total ticks. Replace with a single loop iterating over sub-frames (count = `ticks / local_ticks`), each gathering `local_ticks` audio frames via `std::copy_n`. This eliminates the shadowing, removes a nesting level, and makes the grouping semantics self-documenting.

## 11. `flimcompressor.hpp`: Delete `DitheringParameters` and pass `const encoding_profile &` to `Ditherer`

`DitheringParameters` is a 10-field struct where 9 fields are mechanically copied 1:1 from `encoding_profile` getters. Every new dithering knob added to the profile must be mirrored into the struct, its aggregate init, and every read site. Instead, have `Ditherer` hold a `const encoding_profile &` plus the one outlier (`watermark`) as a separate `std::string` member. This deletes ~20 lines of boilerplate and a maintenance hazard.

## 12. `flimcompressor.hpp`: Rename `penality` to `penalty`

The field name `penality` appears in `codec_spec`, `make_codec`, and `EncodingResult` — it's consistently misspelled throughout. Rename to `penalty` for clarity.

## 13. `flimcompressor.hpp`: Remove `using namespace` at file scope in headers

`flimcompressor.hpp` has `using namespace macflim;` and `using namespace std::string_literals;` at file scope, polluting the namespace of every translation unit that includes it. Replace with qualified names or move the `using` declarations into function/class scope.

## 14. `flimcompressor.hpp`: Fix `#include "profile.hpp"` at the bottom of the file

`profile.hpp` is included *after* the class definition due to a circular dependency (the `compress` method body needs the full `encoding_profile` type, but only a forward declaration is available at the top). The file split from item 5 would naturally resolve this; otherwise, moving the `compress` implementation into a `.cpp` file eliminates the need for the bottom-of-file include hack.
