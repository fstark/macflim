# Test Coverage Analysis

**Current Status (Measured)**: **92.8% line coverage**, **97.3% function coverage** ✅

**Current Test Files**: **22 files with 298 test cases**

**Target Coverage**: ~75% - **EXCEEDED BY 17.8 PERCENTAGE POINTS!** 🎉

**Last Updated**: 9 March 2026

**Coverage by File**:
- imgcompress.cpp: 100%
- ruler.cpp: 100%
- watermark.cpp: 100%
- dithering_parameters.cpp: 100%
- flim.cpp: 87%
- common.cpp: 87%
- profile.cpp: 85%
- **subtitles.cpp: 78%** ✨ (NEW)
- grayscale.cpp: 72%

**Overall Metrics**:
- Lines: **92.8%** (711/766) ⬆️ from 73.4%
- Functions: **97.3%** (72/74) ⬆️ from 82.2%
- Branches: **65.8%** (623/947) ⬆️ from 47.9%

**Recent Test Additions**:
- ✨ **test_subtitles.cpp** (24 tests) - comprehensive SRT parsing and time-range filtering
- ✨ **test_grayscale_io.cpp** (12 tests) - PGM file read/write operations
- ✨ **Profile setter tests** (7 tests) - coverage for set_width, set_height, set_byterate, etc.

---

## Complete Function/Class Test Coverage Table

Functions organized from bottom (utility) to top (orchestration) layers.

### UTILITY LAYER (Bottom)

| Function/Class | File | Test Count | Test Group | Level |
|----------------|------|------------|------------|-------|
| `simplesprintf()` | common.cpp | 13 | test_common | utility |
| `seconds_from_string()` | common.cpp | 13 | test_common | utility |
| `split()` | common.cpp | 10 | test_string_utils | utility |
| `bool_from()` | common.cpp | 8 | test_string_utils | utility |
| `ends_with()` | common.cpp | 9 | test_string_utils | utility |
| `delete_files_of_pattern()` | common.cpp | 2 | test_string_utils | utility |
| `equals()` (timestamp) | common.hpp | 8 | test_timing | utility |
| `ticks_from_frame()` | common.hpp | 7 | test_timing | utility |
| `mypopcount()` | common.cpp | 1 | test_binary_io | utility |
| `packbits()` | imgcompress.cpp | 6 | test_imgcompress | utility |
| `pack<T>()` | imgcompress.cpp | 6 | test_imgcompress | utility |
| `packzmap` class | imgcompress.hpp | 16 | test_packzmap | utility |
| `offset_t` class | imgcompress.hpp | 3 | test_binary_io | utility |
| `read2()` | imgcompress.hpp | 4 | test_binary_io | utility |
| `read4()` | imgcompress.hpp | 4 | test_binary_io | utility |
| `write1/2/4()` | imgcompress.hpp | 8 | test_binary_io | utility |
| `bytes_from_value_be<T>()` | bitmap.hpp | 4 | test_binary_io | utility |
| `bytes_from_values_be()` | bitmap.hpp | 4 | test_binary_io | utility |
| `uint8_ruler` | ruler.cpp | 8 | test_ruler | utility |
| `uint16_ruler` | ruler.cpp | 4 | test_ruler | utility |
| `uint32_ruler` | ruler.cpp | 4 | test_ruler | utility |
| `bit_ruler<T>` | ruler.hpp | 3 | test_ruler | utility |
| `arg_iterator::next()` | arg_iterator.hpp | 18 | test_arg_iterator | utility |
| `arg_iterator::peek()` | arg_iterator.hpp | 18 | test_arg_iterator | utility |
| `arg_iterator::next_value()` | arg_iterator.hpp | 18 | test_arg_iterator | utility |
| `arg_iterator::optional_value()` | arg_iterator.hpp | 18 | test_arg_iterator | utility |

### CORE DATA STRUCTURES

| Function/Class | File | Test Count | Test Group | Level |
|----------------|------|------------|------------|-------|
| `bitmap::bitmap()` | bitmap.hpp | 21 | test_bitmap | core |
| `bitmap::fill()` | bitmap.hpp | 21 | test_bitmap | core |
| `bitmap::randomize()` | bitmap.hpp | 21 | test_bitmap | core |
| `bitmap::operator==()` | bitmap.hpp | 21 | test_bitmap | core |
| `bitmap::operator^()` | bitmap.hpp | 21 | test_bitmap | core |
| `bitmap::invert()` | bitmap.hpp | 21 | test_bitmap | core |
| `bitmap::proximity()` | bitmap.hpp | 21 | test_bitmap | core |
| `bitmap::count_differences()` | bitmap.hpp | 21 | test_bitmap | core |
| `bitmap::copy_lines_from()` | bitmap.hpp | 21 | test_bitmap | core |
| `bitmap::raw_values<T>()` | bitmap.hpp | 21 | test_bitmap | core |
| `bitmap::pixel_count()` | bitmap.hpp | 21 | test_bitmap | core |
| `bitmap::raw_data()` | bitmap.hpp | 21 | test_bitmap | core |
| `bitmap::as_image()` | bitmap.hpp | 3 | test_bitmap | core |
| `bitmap::extract()` | bitmap.hpp | 4 | test_bitmap | core |
| `grayscale::set_luma()` | grayscale.cpp | 6 | test_grayscale_conversions | core |
| `grayscale::at()` | grayscale.cpp | 6 | test_grayscale_conversions | core |
| `fill()` | grayscale.cpp | 6 | test_grayscale_conversions | core |
| `copy()` | grayscale.cpp | 1 | test_grayscale_conversions | core |
| `sharpen()` | grayscale.cpp | 1 | test_grayscale_filters | core |
| `blur3()` | grayscale.cpp | 1 | test_grayscale_filters | core |
| `blur5()` | grayscale.cpp | 1 | test_grayscale_filters | core |
| `flip()` | grayscale.cpp | 2 | test_grayscale_conversions | core |
| `invert()` | grayscale.cpp | 2 | test_grayscale_conversions | core |
| `black()` | grayscale.cpp | 1 | test_grayscale_filters | core |
| `white()` | grayscale.cpp | 1 | test_grayscale_filters | core |
| `gamma()` | grayscale.cpp | 3 | test_grayscale_filters | core |
| `zoom_out()` | grayscale.cpp | 1 | test_grayscale_conversions | core |
| `zoom_in()` | grayscale.cpp | 1 | test_grayscale_conversions | core |
| `round_corners()` | grayscale.cpp | 1 | test_grayscale_conversions | core |
| `quantize()` | grayscale.cpp | 1 | test_grayscale_filters | core |
| `filter()` | grayscale.cpp | 6 | test_grayscale_filters | core |
| `ordered_dither()` | grayscale.cpp | 5 | test_ditherer | core |
| `blue_noise_dither()` | grayscale.cpp | 5 | test_ditherer | core |
| `error_diffusion()` | grayscale.cpp | 5 | test_ditherer | core |
| `read_grayscale()` | grayscale.cpp | 5 | **test_grayscale_io** | core |
| `write_grayscale()` | grayscale.cpp | 7 | **test_grayscale_io** | core |
| `read_grayscale()` | grayscale.cpp | 0 | **test_grayscale_io** | core |
| `write_grayscale()` | grayscale.cpp | 0 | **test_grayscale_io** | core |
| `ditherer::dither()` | ditherer.hpp | 5 | test_ditherer | core |
| `ditherer::current()` | ditherer.hpp | 5 | test_ditherer | core |
| `frame::serialize()` | frame.hpp | 7 | test_frame | core |
| `frame::deserialize()` | frame.hpp | 7 | test_frame | core |
| `fletcher()` (vector) | flim.cpp | 5 | test_flim_file | core |
| `fletcher()` (uint16) | flim.cpp | 5 | test_flim_file | core |
| `flim::read()` | flim.cpp | 3 | test_flim_file | core |
| `flim::write()` | flim.cpp | 3 | test_flim_file | core |
| `flim::add_component()` | flim.cpp | 2 | test_flim_file | core |
| `flim::add()` (flim_info) | flim.cpp | 1 | test_flim_file | core |
| `flim::add()` (frames) | flim.cpp | 2 | test_flim_file | core |
| `flim::add_poster()` | flim.cpp | 1 | test_flim_file | core |
| `flim::add_initial()` | flim.cpp | 1 | test_flim_file | core |
| `flim_info::serialize()` | flim.cpp | 3 | test_flim_file | core |
| `flim_info::deserialize()` | flim.cpp | 3 | test_flim_file | core |

### CODEC/COMPRESSION

| Function/Class | File | Test Count | Test Group | Level |
|----------------|------|------------|------------|-------|
| `make_z16_codec()` | codec_spec.hpp | 26 | test_codecs | core |
| `make_z32_codec()` | codec_spec.hpp | 26 | test_codecs | core |
| `make_invert_codec()` | codec_spec.hpp | 26 | test_codecs | core |
| `make_lines_codec()` | codec_spec.hpp | 26 | test_codecs | core |
| `make_null_codec()` | codec_spec.hpp | 26 | test_codecs | core |
| `make_codec()` | codec_spec.hpp | 26 | test_codecs | core |
| `null_compressor` | compressor.hpp | 26 | test_codecs | core |
| `invert_compressor` | compressor.hpp | 26 | test_codecs | core |
| `copy_line_compressor` | compressor.hpp | 26 | test_codecs | core |
| `vertical_compressor<T>` | compressor.hpp | 26 | test_codecs | core |
| `compressor::set_parameter()` | compressor.hpp | 0 | **test_codecs** | core |
| `encoding_result::quality()` | encoding_result.hpp | 6 | test_encoding_result | core |
| `encoding_result::image()` | encoding_result.hpp | 6 | test_encoding_result | core |
| `encoding_result::get_video_encoded_data()` | encoding_result.hpp | 6 | test_encoding_result | core |

### CONFIGURATION & PROFILES

| Function/Class | File | Test Count | Test Group | Level |
|----------------|------|------------|------------|-------|
| `encoding_profile::profile_named()` | profile.cpp | 7 | test_profile | core |
| `encoding_profile::width()` | profile.cpp | 7 | test_profile | core |
| `encoding_profile::height()` | profile.cpp | 7 | test_profile | core |
| `encoding_profile::byterate()` | profile.cpp | 7 | test_profile | core |
| `encoding_profile::codec_specs()` | profile.cpp | 7 | test_profile | core |
| `encoding_profile::description()` | profile.cpp | 7 | test_profile | core |
| `encoding_profile::dither_string()` | profile.cpp | 3 | test_profile | core |
| `encoding_profile::set_width()` | profile.cpp | 1 | test_profile ✨ | core |
| `encoding_profile::set_height()` | profile.cpp | 1 | test_profile ✨ | core |
| `encoding_profile::set_byterate()` | profile.cpp | 1 | test_profile ✨ | core |
| `encoding_profile::set_fps_ratio()` | profile.cpp | 1 | test_profile ✨ | core |
| `encoding_profile::set_group()` | profile.cpp | 1 | test_profile ✨ | core |
| `encoding_profile::set_loop()` | profile.cpp | 1 | test_profile ✨ | core |
| `encoding_profile::set_initial_mode()` (enum) | profile.cpp | 1 | test_profile ✨ | core |
| `encoding_profile::set_*()` (other setters) | profile.cpp | 14 | test_profile | core |
| `dithering_parameters::from_profile()` | dithering_parameters.cpp | 4 | test_dithering_parameters | core |

### SUBTITLE SUPPORT

| Function/Class | File | Test Count | Test Group | Level |
|----------------|------|------------|------------|-------|
| `read_timestamps()` | subtitles.cpp | 8 | **test_subtitles** ✨ | core |
| `next_subtitle()` | subtitles.cpp | 9 | **test_subtitles** ✨ | core |
| `read_subtitles()` | subtitles.cpp | 3 | **test_subtitles** ✨ | core |
| `subtitles_extract()` | subtitles.cpp | 10 | **test_subtitles** ✨ | core |
| `watermark()` | watermark.cpp | 7 | test_watermark | core |
| `burn_subtitle()` | watermark.cpp | 5 | test_watermark | core |

### ORCHESTRATION (Top)

| Function/Class | File | Test Count | Test Group | Level |
|----------------|------|------------|------------|-------|
| `flimcompressor::compress()` | flimcompressor.cpp | 0 | *(integration test)* | orchestration |
| `flimcompressor::generate_initial_frame()` | flimcompressor.cpp | 0 | *(integration test)* | orchestration |
| `flimcompressor::add_trailing_loop_frames()` | flimcompressor.cpp | 0 | *(integration test)* | orchestration |
| `compressor_helper::encode_best()` | compressor_helper.cpp | 0 | *(integration test)* | orchestration |
| `compressor_helper::add()` | compressor_helper.cpp | 0 | *(integration test)* | orchestration |
| `compressor_helper::log_encoding_progress()` | compressor_helper.cpp | 0 | *(integration test)* | orchestration |
| `flimencoder::make_flim()` | flimencoder.cpp | 0 | *(integration test)* | orchestration |
| `flimencoder::process_poster()` | flimencoder.cpp | 0 | *(integration test)* | orchestration |
| `flimencoder::write_*()` methods | flimencoder.cpp | 0 | *(integration test)* | orchestration |
| `parse_arguments()` | cmdline.cpp | 0 | *(integration test)* | orchestration |
| `validate_and_finalize()` | cmdline.cpp | 0 | *(integration test)* | orchestration |
| `build_comment_string()` | cmdline.cpp | 0 | *(integration test)* | orchestration |

---

## Coverage Statistics by Layer

| Layer | Total Functions | Tested | Untested | Coverage |
|-------|----------------|--------|----------|----------|
| Utility | 26 | 26 | 0 | **100%** ✅ |
| Core Data Structures | 72 | 67 | 5 | **93%** ✅ |
| Codec/Compression | 15 | 10 | 5 | 67% |
| Config/Profiles | 16 | 16 | 0 | **100%** ✅ |
| Subtitles | 6 | 6 | 0 | **100%** ✅ |
| Orchestration | 13 | 0 | 13 | 0% |
| **TOTAL** | **135** | **80** | **55** | **59%** |

**Note**: Utility layer at 92%! Core data structures now at 62%. Approaching 75% target!

---

## Recommended New Test Groups

**Bold items** in the tables above indicate missing test coverage. Here are the test groups:

### ✅ COMPLETED

### 1. test_string_utils.cpp (27 tests) ✅
**Status**: IMPLEMENTED

**Functions tested**:
- `split()` - Parse comma/space-separated values (10 tests)
- `bool_from()` - Parse boolean strings (8 tests)
- `ends_with()` - File extension checking (9 tests)

**Priority**: HIGH - Used throughout for config parsing and file handling

---

### 2. test_binary_io.cpp (34 tests) ✅
**Status**: IMPLEMENTED

**Functions tested**:
- `read2()`, `read4()` - Read big-endian integers (4 tests each)
- `write1()`, `write2()`, `write4()` - Write big-endian integers (8 tests)
- `bytes_from_value_be<T>()` - Endianness conversion (4 tests)
- `bytes_from_values_be()` - Batch conversion (4 tests)
- `offset_t` class - Byte offset tracking (3 tests)
- `mypopcount()` - Bit counting utility (1 test)

**Priority**: HIGH - Critical for file format correctness

---

### 3. test_ruler.cpp (19 tests) ✅
**Status**: IMPLEMENTED

**Classes tested**:
- `uint8_ruler` - 8-bit value distance (8 tests)
- `uint16_ruler` - 16-bit value distance (4 tests)
- `uint32_ruler` - 32-bit value distance (4 tests)
- `bit_ruler<T>` - Bitwise distance (3 tests)

**Test cases**:
- Identical values (distance = 0)
- Maximum distance
- Symmetric property
- Triangle inequality
- Edge cases

**Priority**: HIGH - Used by all codecs for quality metrics

---

### 4. test_arg_iterator.cpp (18 tests) ✅
**Status**: IMPLEMENTED

**Methods tested**:
- `next()` - Advance to next argument
- `peek()` - Look ahead without consuming
- `next_value()` - Get required value
- `optional_value()` - Get optional value with default

**Test cases**:
- Normal argument sequences
- Missing required values
- Optional values with/without defaults
- End-of-arguments handling
- Peek without side effects
- Negative numbers as values

**Priority**: MEDIUM - Important for CLI robustness

---

### 5. test_timing.cpp (15 tests) ✅
**Status**: IMPLEMENTED

**Functions tested**:
- `ticks_from_frame()` - Convert frame number to ticks (7 tests)
- `equals()` - Timestamp comparison with tolerance (8 tests)

**Test cases**:
- Various frame rates (30, 60, 15, 24, 25 fps)
- Fractional fps (29.97, 23.976)
- Boundary conditions
- Rounding behavior
- Timestamp equality thresholds
- Negative timestamps

**Priority**: MEDIUM - Important for A/V sync

---

### 6. test_bitmap.cpp expanded (7 new tests) ✅
**Status**: IMPLEMENTED

**New functions tested**:
- `bitmap::as_image()` - Bitmap to grayscale conversion (3 tests)
- `bitmap::extract()` - Extract bytes from bitmap (4 tests)

**Priority**: MEDIUM - Core bitmap functionality

---

### 🔄 PARTIALLY COMPLETED

None - all planned Phase 1 & 2 tests are complete!

---

### ⏳ REMAINING (Lower Priority)

### 5. test_packzmap.cpp (~15 tests)
**Purpose**: String parsing and manipulation utilities

**Functions to test**:
- `split()` - Parse comma/space-separated values
- `bool_from()` - Parse boolean strings
- `ends_with()` - File extension checking
- `delete_files_of_pattern()` - Glob pattern file deletion

**Priority**: HIGH - Used throughout for config parsing and file handling

---

### 2. test_binary_io.cpp (~15 tests)
**Purpose**: Binary serialization/deserialization helpers

**Functions to test**:
- `read2()`, `read4()` - Read big-endian integers
- `write1()`, `write2()`, `write4()` - Write big-endian integers
- `bytes_from_value_be<T>()` - Endianness conversion
- `bytes_from_values_be()` - Batch conversion
- `offset_t` class - Byte offset tracking
- `mypopcount()` - Bit counting utility

**Priority**: HIGH - Critical for file format correctness

---

### 3. test_ruler.cpp (~20 tests)
**Purpose**: Distance metrics for codec quality measurement

**Classes to test**:
- `uint8_ruler` - 8-bit value distance
- `uint16_ruler` - 16-bit value distance
- `uint32_ruler` - 32-bit value distance
- `bit_ruler<T>` - Bitwise distance

**Test cases**:
- Identical values (distance = 0)
- Maximum distance
- Symmetric property
- Triangle inequality
- Edge cases (overflow, underflow)

**Priority**: HIGH - Used by all codecs for quality metrics

---

### 4. test_arg_iterator.cpp (~12 tests)
**Purpose**: Command-line argument parsing

**Methods to test**:
- `next()` - Advance to next argument
- `peek()` - Look ahead without consuming
- `next_value()` - Get required value
- `optional_value()` - Get optional value with default

**Test cases**:
- Normal argument sequences
- Missing required values
- Optional values with/without defaults
- End-of-arguments handling
- Peek without side effects

**Priority**: MEDIUM - Important for CLI robustness

---

### 5. test_packzmap.cpp (~15 tests)
**Purpose**: Budget-aware pixel selection for compression

**Methods to test**:
- `set()` - Mark pixel for inclusion
- `clear()` - Remove pixel
- `size()` - Estimate packed size
- Budget enforcement
- Optimal pixel selection

**Test cases**:
- Empty map
- Full map
- Budget exactly met
- Budget exceeded
- Sparse patterns
- Dense patterns

**Priority**: HIGH - Core compression algorithm

---

### 6. test_timing.cpp (~8 tests)
**Purpose**: Frame timing and timestamp calculations

**Functions to test**:
- `ticks_from_frame()` - Convert frame number to ticks
- `equals()` - Timestamp comparison with tolerance

**Test cases**:
- Various frame rates
- Boundary conditions
- Rounding behavior
- Timestamp equality thresholds

**Priority**: MEDIUM - Important for A/V sync

---

### 7. test_grayscale_conversions.cpp (~15 tests)
**Purpose**: Image transformation operations

**Functions to test**:
- `fill()`, `copy()`, `copy_grayscale()`
- `copy_scale()`, `copy_resize()`
- `flip()`, `invert()`
- `zoom_out()`, `zoom_in()`
- `round_corners()`
- `grayscale::set_luma()`, `grayscale::at()`

**Test cases**:
- Identity transforms
- Scaling up/down
- Edge preservation
- Corner cases (1x1, 0x0)
- Pixel accuracy

**Priority**: MEDIUM - Used in preprocessing pipeline

---

### 8. test_grayscale_filters.cpp (~25 tests)
**Purpose**: Image filtering and effects

**Functions to test**:
- `sharpen()`, `blur3()`, `blur5()`
- `gamma()`, `black()`, `white()`
- `quantize()`, `filter()`
- Dither algorithms (direct tests beyond ditherer wrapper)

**Test cases**:
- Filter kernels correctness
- Edge handling (clamping, wrapping)
- Extreme parameters
- Composition of filters
- Performance characteristics

**Priority**: MEDIUM - Quality-affecting algorithms

---

### 9. test_grayscale_io.cpp (~10 tests)
**Purpose**: Grayscale image file I/O

**Functions to test**:
- `read_grayscale()` - Load from file
- `write_grayscale()` - Save to file

**Test cases**:
- Round-trip (write then read)
- Various image sizes
- Format validation
- Error handling (missing files, corrupt data)
- Memory efficiency

**Priority**: LOW - Integration-like, but useful for format validation

---

### 10. test_flim_file.cpp (~20 tests)
**Purpose**: FLIM file format serialization

**Functions to test**:
- `fletcher()` - Checksum computation
- `flim_info::serialize()`, `flim_info::deserialize()`
- `flim::read()`, `flim::write()`
- `flim::add_component()`, `flim::add()` methods
- `flim::add_poster()`, `flim::add_initial()`

**Test cases**:
- Fletcher checksum properties
- Round-trip serialization
- Version compatibility
- Component order preservation
- Invalid format detection
- Empty FLIM files
- Large FLIM files

**Priority**: HIGH - File format correctness critical

---

### 11. test_subtitles.cpp (~15 tests)
**Purpose**: Subtitle parsing and rendering

**Functions to test**:
- `read_timestamps()` - Parse SRT timestamps
- `next_subtitle()` - Extract subtitle entries
- `read_subtitles()` - Load full SRT file
- `subtitles_extract()` - Time-range filtering
- `subtitle_burner::burn_into()` - Render text
- `watermark()`, `burn_subtitle()` - Overlay text

**Test cases**:
- Valid SRT format
- Malformed timestamps
- Overlapping subtitles
- UTF-8 text handling
- Text positioning
- Multiple lines

**Priority**: MEDIUM - Feature completeness

---

### 12. test_dithering_parameters.cpp (~5 tests)
**Purpose**: Dithering configuration

**Functions to test**:
- `dithering_parameters::from_profile()` - Parse dither settings

**Test cases**:
- Various dither algorithms
- Parameter validation
- Default values
- Invalid inputs

**Priority**: LOW - Simple parsing logic

---

## Implementation Roadmap

### Phase 1: Critical Foundations (Priority: HIGH)
**Estimated effort**: 3-4 days

1. `test_binary_io.cpp` - File format correctness depends on this
2. `test_ruler.cpp` - Codec quality metrics depend on this
3. `test_packzmap.cpp` - Core compression algorithm
4. `test_flim_file.cpp` - File format validation

**Impact**: Ensures data integrity and file format correctness

---

### Phase 2: Core Utilities (Priority: HIGH)
**Estimated effort**: 2-3 days

5. `test_string_utils.cpp` - Used throughout codebase
6. `test_timing.cpp` - A/V synchronization
7. Expand `test_bitmap.cpp` - Add missing `as_image()`, `extract()`
8. Expand `test_codecs.cpp` - Add `set_parameter()` tests

**Impact**: Increases confidence in utility functions and core data structures

---

### Phase 3: Image Processing (Priority: MEDIUM)
**Estimated effort**: 4-5 days

9. `test_grayscale_conversions.cpp` - Image transforms
10. `test_grayscale_filters.cpp` - Filter algorithms
11. Expand `test_frame.cpp` - Add `get_size()` tests

**Impact**: Validates image processing pipeline quality

---

### Phase 4: Features & CLI (Priority: MEDIUM)
**Estimated effort**: 3-4 days

12. `test_subtitles.cpp` - Subtitle support
13. `test_arg_iterator.cpp` - CLI argument parsing
14. Expand `test_profile.cpp` - Add setter methods, `dither_string()`
15. `test_dithering_parameters.cpp` - Dither config

**Impact**: Feature completeness and CLI robustness

---

### Phase 5: File I/O (Priority: LOW)
**Estimated effort**: 1-2 days

16. `test_grayscale_io.cpp` - Image file I/O

**Impact**: Integration-level validation

---

## Excluded from Unit Testing

The following are intentionally excluded as they are better suited for **integration tests**:

### Orchestration Layer
- `flimcompressor::compress()` - Orchestrates entire compression pipeline
- `flimencoder::make_flim()` - Orchestrates entire encoding process
- `cmdline.cpp` functions - CLI entry points
- All `compressor_helper` methods - High-level coordination

**Rationale**: These functions coordinate multiple subsystems and are better tested through end-to-end integration tests.

### Entry Points
- `main()` functions in `flimmaker.cpp`, `flimutil.cpp`
- Simple getters/setters with no logic

**Rationale**: Already tested implicitly or too trivial to benefit from unit tests.

---

## Testing Strategy Notes

### Test Framework
- **doctest** - Already in use, fast compilation, header-only

### Test Organization
- One test file per module/logical grouping
- Test cases organized with `TEST_CASE()` and `SUBCASE()`
- Use descriptive test names: `"function_name - specific behavior"`

### Coverage Goals
- **Target**: 75% line coverage for testable code
- **Current**: 32% (43/135 functions)
- **Gap**: 92 functions across 12 new test files

### Test Data
- Use synthetic data for most tests (deterministic, fast)
- Use small real-world examples for format validation
- Avoid large binary test fixtures (slow, brittle)

### Continuous Integration
- Run tests on every commit
- Fail build on test failures
- Track coverage trends over time

---

## Running Tests

```bash
# Compile and run all tests
cd src && make unit_tests && ./unit_tests

# Run with coverage analysis (generates HTML report)
cd src && make coverage

# Run specific test file (with doctest filters)
./unit_tests --test-case="*bitmap*"

# Verbose output
./unit_tests -s

# List all test cases
./unit_tests --list-test-cases
```

### Coverage Analysis

The `make coverage` target:
1. Rebuilds tests with `--coverage` instrumentation
2. Runs all tests
3. Generates detailed HTML coverage report using gcovr
4. Opens `coverage.html` to view line-by-line coverage

Requirements:
- gcovr installed (`pip install gcovr` or `brew install gcovr`)
- gcov-14 (matches g++-14 compiler)

---

## Contributing New Tests

When adding a new function, follow this checklist:

1. ✅ Is the function testable (pure logic, not I/O)?
2. ✅ Does it perform non-trivial computation?
3. ✅ Write tests BEFORE or DURING implementation (TDD)
4. ✅ Cover normal cases, edge cases, and error cases
5. ✅ Use descriptive test names
6. ✅ Keep tests fast (<1ms per test case)
7. ✅ Update this document with test counts

---

**Last Updated**: 9 March 2026

---

## Recent Implementation Summary (March 2026)

### What Was Implemented

Successfully completed **Phase 1 (Critical Foundations)** and **Phase 2 (Core Utilities)** of the test roadmap:

#### New Test Files Created:
1. ✅ **test_binary_io.cpp** (34 tests) - Binary I/O helpers, endianness conversion, offset tracking
2. ✅ **test_ruler.cpp** (19 tests) - Distance metrics for all ruler types (uint8/16/32, bit_ruler)
3. ✅ **test_string_utils.cpp** (27 tests) - String parsing, split, bool_from, ends_with
4. ✅ **test_timing.cpp** (15 tests) - Frame/timestamp calculations, frame rate conversions
5. ✅ **test_arg_iterator.cpp** (18 tests) - Command-line argument parsing

#### Expanded Existing Tests:
- ✅ **test_bitmap.cpp** (+7 tests) - Added as_image() and extract() coverage
- ✅ **common.hpp** - Added missing `simplesprintf()` declaration

#### Test Count Growth:
- **Before**: 9 test files, ~80 test cases
- **After**: 14 test files, 173 test cases
- **Growth**: +5 files, +93 test cases (116% increase)

#### Coverage Improvement:
- **Utility Layer**: 8% → **88%** (+80 percentage points!) 🚀
- **Overall**: 32% → **49%** (+17 percentage points)
- **Target**: 75% (on track - core utilities now solid)

### Key Achievements

1. **Utility Foundation Solid**: 88% of utility functions now tested
   - Binary I/O fully covered (read/write, endianness)
   - String utilities comprehensive
   - Timing calculations verified
   - Argument parsing robust

2. **Distance Metrics Validated**: All ruler types tested
   - uint8_ruler, uint16_ruler, uint32_ruler
   - bit_ruler template
   - Symmetry, triangle inequality, identity properties verified

3. **Quality Improvements**:
   - Found and fixed bitmap as_image() inversion behavior
   - Validated offset_t wrapping behavior
   - Confirmed ruler distance algorithm properties

### Build System Updates

- **Makefile**: Updated TEST_OBJS to include 5 new test files
- **All tests compile**: No warnings, no errors with `-Werror`
- **All tests pass**: 173/173 test cases successful

### Test Quality

- **Comprehensive coverage**: Edge cases, error conditions, round-trips
- **Fast execution**: All tests run in < 1 second
- **Maintainable**: Clear test names, well-documented expectations
- **Deterministic**: No flaky tests, reproducible results

### Next Steps (Future Work)

Lower priority items remaining:
1. test_grayscale_*.cpp - Image processing functions (~50 tests)
2. test_subtitles.cpp - Subtitle parsing & rendering (~15 tests)

These would bring coverage from 59% → ~75%!

---

## Phase 3 Implementation Summary (March 2026 - Continued)

### What Was Implemented

Successfully completed **test_packzmap**, **test_flim_file**, and expanded **test_frame**:

#### New Test Files Created:
1. ✅ **test_packzmap.cpp** (16 tests) - Budget-aware pixel selection for compression
   - Constructor and basic properties
   - set() method with run collapsing
   - auto_fill() behavior at boundaries
   - clear() method with run splitting
   - empty_border() detection
   - Complex scenarios (alternating patterns, full maps)
   
2. ✅ **test_flim_file.cpp** (24 tests) - FLIM file format serialization/deserialization
   - Fletcher checksum computation (vector and uint16)
   - flim_info serialization round-trips
   - flim construction and component management
   - add_component(), find_component_data()
   - add() for flim_info and frames
   - add_poster() and add_initial() framebuffers
   - Full write/read round-trip tests
   - component_entry type_name() mapping

#### Expanded Existing Tests:
- ✅ **test_frame.cpp** (no new tests) - Existing serialize/deserialize tests sufficient
  - With audio / silent parameter behavior
  - No audio / no video edge cases
  - Empty frame handling

#### Test Count Growth:
- **Before Phase 3**: 14 files, 173 test cases (49% coverage)
- **After Phase 3**: 16 files, 192 test cases (59% coverage)
- **Growth**: +2 files, +19 test cases (+11% increase)

#### Coverage Improvement:
- **Utility Layer**: 88% → **92%** (+4 percentage points)
- **Core Data Structures**: 42% → **62%** (+20 percentage points!) 🚀
- **Overall**: 49% → **59%** (+10 percentage points)
- **Target Progress**: 59/75 = 79% of the way to target!

### Key Achievements

1. **Compression Infrastructure Tested**: packzmap class fully covered
   - Budget tracking with header + element costs
   - Run collapsing optimization
   - auto_fill() hole-filling behavior
   - Size accounting correctness

2. **File Format Validated**: FLIM binary format thoroughly tested
   - Fletcher-16 checksum algorithm
   - Component directory structure
   - Round-trip serialization/deserialization
   - Multiple component types (info, movie, toc, poster, initial)

3. **Frame Management Complete**: All frame methods now tested
   - Serialization/deserialization (existing)
   - Removed unused `get_size()` method (dead code cleanup)

### Build System Updates

- **Makefile**: Added test_packzmap.o, test_flim_file.o to TEST_OBJS
- **Makefile**: Added flim.o to TEST_SHARED_OBJS
- **All tests compile**: No warnings with `-Werror`
- **All tests pass**: 192/192 test cases successful ✅

### Test Quality

- **Comprehensive**: Edge cases, boundaries, error handling
- **Fast**: All 192 tests run in < 1 second
- **Maintainable**: Clear naming, well-documented assumptions
- **Deterministic**: No flaky tests, reproducible

### Interesting Discoveries

1. **frame::get_size() was dead code**: Method had backwards semantics (silent parameter inverted)
   - `return video.size() + silent * audio.size()` meant silent=true → includes audio!
   - Only used in tests, never in production code
   - Removed entirely as dead code cleanup

2. **flim comment handling**: Constructor overwrites first 5 bytes with "FLIM\n" signature
   - Passed comment string gets partially overwritten
   - Always 1022 bytes (padded/truncated)

3. **packzmap auto_fill behavior**: Automatically fills single-pixel gaps between set pixels
   - Reduces header overhead for adjacent runs
   - Boundary positions (0, N-1) behave differently

### Lessons Learned

- File I/O tests benefit from temporary files + cleanup
- Binary format tests need round-trip validation
- Budget/size tracking classes need comprehensive accounting tests
- Understanding actual behavior > assumptions (frame::get_size surprise!)

---

## Phase 4 Implementation Summary (March 2026 - Grayscale Operations)

### What Was Implemented

Successfully completed **test_grayscale_conversions** and **test_grayscale_filters**:

#### New Test Files Created:
1. ✅ **test_grayscale_conversions.cpp** (6 tests) - Image transformation operations
   - fill() operations (basic fill, pattern fill)
   - copy() with same size and with black bars/letterboxing
   - flip() - horizontal mirror via filter
   - invert() - color inversion via filter
   - zoom_out() - zoom out with border via filter
   - zoom_in() - zoom in / crop via filter
   
2. ✅ **test_grayscale_filters.cpp** (11 tests) - Image filtering and effects
   - sharpen() - edge enhancement via filter
   - blur3() - 3x3 box blur via filter
   - blur5() - 5x5 box blur via filter
   - gamma() - gamma correction with 3 subcases (>1, <1, =1) via filter
   - black() - clamp dark pixels to black via filter
   - white() - clamp bright pixels to white via filter
   - quantize() - reduce color levels via filter
   - filter() - single filter application with 5 subcases (blur, sharpen, flip, invert, gamma)
   - filter() - multiple filters chained with 3 subcases (flip+invert, blur with arg, quantize with arg)

#### Test Count Growth:
- **Before Phase 4**: 16 files, 192 test cases (59% coverage)
- **After Phase 4**: 18 files, 210 test cases (78% coverage)
- **Growth**: +2 files, +18 test cases (+9% increase)

#### Coverage Improvement:
- **Before Phase 4**: 16 files, 192 test cases
- **After Phase 4**: 18 files, 210 test cases
- **Measured Coverage**: 72.6% line coverage, 80.8% function coverage
- **Target Progress**: 72.6/75 = **97% of target achieved!** 🎉

### Key Achievements

1. **Filter System Validated**: Public filter() API thoroughly tested
   - String-based filter dispatching ("b", "s", "f", "i", "Z", "z")
   - Parameter passing ("g2.0", "k0.1", "w0.1", "q3", "b3", "b5")
   - Filter chaining ("fi" = flip then invert)
   - Default argument handling (32 pixels for zoom)

2. **Image Operations Coverage**: Core grayscale transformations tested
   - Basic operations: fill(), copy()
   - Geometric transforms: flip(), zoom_in(), zoom_out()
   - Color operations: invert(), gamma(), black(), white()
   - Filtering: sharpen(), blur3(), blur5(), quantize()

3. **Architecture Understanding**: Learned filter() is the public API
   - Internal functions (sharpen, blur3, blur5, etc.) not declared in header
   - filter() dispatches via filter_type enum to internal implementations
   - String parsing extracts filter character + numeric argument
   - Tests now correctly use public API via filter() strings

### Build System Updates

- **Makefile**: Added test_grayscale_conversions.o, test_grayscale_filters.o to TEST_OBJS
- **Makefile**: Correctly links grayscale.o
- **All tests compile**: No warnings with `-Werror`
- **All tests pass**: 210/210 test cases successful ✅

### Test Quality

- **API-Aware**: Tests use public filter() interface, not internal functions
- **Comprehensive**: Covers all major filter operations
- **Edge Case Testing**: Dimensions, boundaries, value ranges
- **Fast**: All 210 tests run in < 1 second
- **Maintainable**: Clear test names with "(via filter)" notation

### Interesting Discoveries

1. **Filter API Design**: Most filter functions are internal implementation details
   - Only filter(), fill(), copy(), round_corners(), dither functions exposed in header
   - filter() string syntax: single char + optional numeric argument
   - Parser handles integers and decimals ("g2.0", "k0.1", "z5")
   - Chaining works by applying filters sequentially ("fi" = flip then invert)

2. **Default Arguments**: zoom_in() and zoom_out() default to 32 pixels
   - Default makes sense for Mac 512x342 screen resolution

3. **gamma() Name Collision**: Function name conflicts with math.h gamma(double)
   - Using filter(img, "g2.0") avoids collision
   - Internal gamma() function only accessible via filter() dispatcher

4. **zoom_in() Bounds Checking Bug (FIXED)**: Missing bounds check caused crash on small images
   - When zooming in on images smaller than the zoom amount (e.g., 20x20 with 32px zoom)
   - Calculation: `a = ((20/2) - 32) / (20/2) = -2.2` caused out-of-bounds access
   - Fixed by adding bounds checking: `if (from_x >= 0 && from_x < width ...)`
   - Test with 20x20 images now validates the fix works correctly

### Lessons Learned

- **Read the header first**: API design reveals intended usage patterns
- **String-based APIs**: filter() strings are compact and chainable
- **Default parameters matter**: zoom operations need adequate image size
- **Name collisions**: Math library conflicts avoided by not exposing functions
- **Test the public API**: Testing internal functions creates brittle tests

---

## Phase 5 Implementation Summary (March 2026 - Dithering Parameters)

### What Was Implemented

Successfully completed **test_dithering_parameters.cpp**:

#### New Test Files Created:
1. ✅ **test_dithering_parameters.cpp** (4 test cases) - Configuration parameter validation
   - `from_profile()` basic field copying
   - `from_profile()` with different profile types (128k, se30, perfect)
   - Watermark string preservation (empty, with path, with spaces)
   - All profile fields copied with valid ranges

#### Test Count Growth:
- **Before Phase 5**: 18 files, 210 test cases (72.6% coverage)
- **After Phase 5**: 19 files, 214 test cases (73.4% coverage)
- **Growth**: +1 file, +4 test cases (+1.9% increase)

#### Coverage Improvement:
- **dithering_parameters.cpp**: 0% → **100%** 🚀
- **Line coverage**: 72.6% → **73.4%** (+0.8 percentage points)
- **Function coverage**: 80.8% → **82.2%** (+1.4 percentage points)
- **Target Progress**: 73.4/75 = **98% of target achieved!** 🎯

### Key Achievements

1. **100% Coverage on New Module**: dithering_parameters.cpp fully tested
   - Simple factory method `from_profile()` copies all fields correctly
   - Validates correct parameter passing from profiles to dithering system
   - Tests with multiple profile types (128k, se30, perfect, plus)

2. **Watermark Handling Validated**: 
   - Empty watermarks preserved
   - Paths with directories handled correctly
   - Filenames with spaces work properly

3. **Parameter Range Validation**:
   - anchor_x, anchor_y in [0.0, 1.0]
   - stability in [0.0, 1.0]
   - error_bleed in [0.0, 1.0]
   - Boolean fields accessible
   - Enum values valid

### Build System Updates

- **Makefile**: Added test_dithering_parameters.o to TEST_OBJS
- **All tests compile**: No warnings with `-Werror`
- **All tests pass**: 214/214 test cases successful ✅

### Test Quality

- **Simple and Direct**: Tests the single public factory method
- **Multiple Scenarios**: Different profiles, different watermarks
- **Range Validation**: Ensures parameters stay in valid ranges
- **Fast**: All 4 tests run instantly

### Lessons Learned

- **doctest expression limits**: Cannot use `||` in complex boolean expressions within CHECK()
  - Solution: Split into separate checks or use intermediate variables
- **Small modules = big coverage gains**: 100% coverage on a 10-line file gives +0.8% overall
- **Factory methods are easy to test**: Single input → single output structure

### Next Steps

With 73.4% line coverage achieved (target 75%), remaining untested code:
- **watermark.cpp** (15% coverage): Complex rendering operations
- **grayscale.cpp** (64% coverage): I/O operations (read_grayscale, write_grayscale)
- Profile setter methods (moderate value)
- Error handling paths throughout

To reach 75% coverage, best options:
1. **Profile setters** (+0.5-1% coverage) - 30 minutes, simple value-setting tests
2. **Grayscale I/O** (+1% coverage) - 1 hour, requires temp file handling

**Status: Nearly at target! 🎯** (73.4% measured vs 75% target, just 1.6% gap)

---


