# Test Coverage Analysis

**Current Status**: 43 functions tested out of ~135 testable functions (32% coverage)

**Current Test Files**: 9 files with ~80 test cases

**Target Coverage**: ~75% with additional 170 test cases across 12 new test groups

---

## Complete Function/Class Test Coverage Table

Functions organized from bottom (utility) to top (orchestration) layers.

### UTILITY LAYER (Bottom)

| Function/Class | File | Test Count | Test Group | Level |
|----------------|------|------------|------------|-------|
| `simplesprintf()` | common.cpp | 13 | test_common | utility |
| `seconds_from_string()` | common.cpp | 13 | test_common | utility |
| `split()` | common.cpp | 0 | **test_string_utils** | utility |
| `bool_from()` | common.cpp | 0 | **test_string_utils** | utility |
| `ends_with()` | common.cpp | 0 | **test_string_utils** | utility |
| `delete_files_of_pattern()` | common.cpp | 0 | **test_string_utils** | utility |
| `equals()` (timestamp) | common.hpp | 0 | **test_timing** | utility |
| `ticks_from_frame()` | common.hpp | 0 | **test_timing** | utility |
| `mypopcount()` | common.cpp | 0 | **test_binary_io** | utility |
| `packbits()` | imgcompress.cpp | 6 | test_imgcompress | utility |
| `pack<T>()` | imgcompress.cpp | 6 | test_imgcompress | utility |
| `packzmap` class | imgcompress.hpp | 0 | **test_packzmap** | utility |
| `offset_t` class | imgcompress.hpp | 0 | **test_binary_io** | utility |
| `read2()` | imgcompress.hpp | 0 | **test_binary_io** | utility |
| `read4()` | imgcompress.hpp | 0 | **test_binary_io** | utility |
| `write1/2/4()` | imgcompress.hpp | 0 | **test_binary_io** | utility |
| `bytes_from_value_be<T>()` | bitmap.hpp | 0 | **test_binary_io** | utility |
| `bytes_from_values_be()` | bitmap.hpp | 0 | **test_binary_io** | utility |
| `uint8_ruler` | ruler.cpp | 0 | **test_ruler** | utility |
| `uint16_ruler` | ruler.cpp | 0 | **test_ruler** | utility |
| `uint32_ruler` | ruler.cpp | 0 | **test_ruler** | utility |
| `bit_ruler<T>` | ruler.hpp | 0 | **test_ruler** | utility |
| `arg_iterator::next()` | arg_iterator.hpp | 0 | **test_arg_iterator** | utility |
| `arg_iterator::peek()` | arg_iterator.hpp | 0 | **test_arg_iterator** | utility |
| `arg_iterator::next_value()` | arg_iterator.hpp | 0 | **test_arg_iterator** | utility |
| `arg_iterator::optional_value()` | arg_iterator.hpp | 0 | **test_arg_iterator** | utility |

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
| `bitmap::as_image()` | bitmap.hpp | 0 | **test_bitmap** | core |
| `bitmap::extract()` | bitmap.hpp | 0 | **test_bitmap** | core |
| `grayscale::set_luma()` | grayscale.cpp | 0 | **test_grayscale_conversions** | core |
| `grayscale::at()` | grayscale.cpp | 0 | **test_grayscale_conversions** | core |
| `fill()` | grayscale.cpp | 0 | **test_grayscale_conversions** | core |
| `copy()` | grayscale.cpp | 0 | **test_grayscale_conversions** | core |
| `copy_grayscale()` | grayscale.cpp | 0 | **test_grayscale_conversions** | core |
| `copy_scale()` | grayscale.cpp | 0 | **test_grayscale_conversions** | core |
| `copy_resize()` | grayscale.cpp | 0 | **test_grayscale_conversions** | core |
| `sharpen()` | grayscale.cpp | 0 | **test_grayscale_filters** | core |
| `blur3()` | grayscale.cpp | 0 | **test_grayscale_filters** | core |
| `blur5()` | grayscale.cpp | 0 | **test_grayscale_filters** | core |
| `flip()` | grayscale.cpp | 0 | **test_grayscale_conversions** | core |
| `invert()` | grayscale.cpp | 0 | **test_grayscale_conversions** | core |
| `black()` | grayscale.cpp | 0 | **test_grayscale_filters** | core |
| `white()` | grayscale.cpp | 0 | **test_grayscale_filters** | core |
| `gamma()` | grayscale.cpp | 0 | **test_grayscale_filters** | core |
| `zoom_out()` | grayscale.cpp | 0 | **test_grayscale_conversions** | core |
| `zoom_in()` | grayscale.cpp | 0 | **test_grayscale_conversions** | core |
| `round_corners()` | grayscale.cpp | 0 | **test_grayscale_conversions** | core |
| `quantize()` | grayscale.cpp | 0 | **test_grayscale_filters** | core |
| `filter()` | grayscale.cpp | 0 | **test_grayscale_filters** | core |
| `ordered_dither()` | grayscale.cpp | 5 | test_ditherer | core |
| `blue_noise_dither()` | grayscale.cpp | 5 | test_ditherer | core |
| `error_diffusion()` | grayscale.cpp | 5 | test_ditherer | core |
| `read_grayscale()` | grayscale.cpp | 0 | **test_grayscale_io** | core |
| `write_grayscale()` | grayscale.cpp | 0 | **test_grayscale_io** | core |
| `ditherer::dither()` | ditherer.hpp | 5 | test_ditherer | core |
| `ditherer::current()` | ditherer.hpp | 5 | test_ditherer | core |
| `frame::serialize()` | frame.hpp | 7 | test_frame | core |
| `frame::deserialize()` | frame.hpp | 7 | test_frame | core |
| `frame::get_size()` | frame.hpp | 0 | **test_frame** | core |
| `fletcher()` (vector) | flim.cpp | 0 | **test_flim_file** | core |
| `fletcher()` (uint16) | flim.cpp | 0 | **test_flim_file** | core |
| `flim::read()` | flim.cpp | 0 | **test_flim_file** | core |
| `flim::write()` | flim.cpp | 0 | **test_flim_file** | core |
| `flim::add_component()` | flim.cpp | 0 | **test_flim_file** | core |
| `flim::add()` (flim_info) | flim.cpp | 0 | **test_flim_file** | core |
| `flim::add()` (frames) | flim.cpp | 0 | **test_flim_file** | core |
| `flim::add_poster()` | flim.cpp | 0 | **test_flim_file** | core |
| `flim::add_initial()` | flim.cpp | 0 | **test_flim_file** | core |
| `flim_info::serialize()` | flim.cpp | 0 | **test_flim_file** | core |
| `flim_info::deserialize()` | flim.cpp | 0 | **test_flim_file** | core |

### CODEC/COMPRESSION

| Function/Class | File | Test Count | Test Group | Level |
|----------------|------|------------|------------|-------|
| `make_z16_codec()` | codec_spec.hpp | 26 | test_codecs | core |
| `make_z32_codec()` | codec_spec.hpp | 26 | test_codecs | core |
| `make_invert_codec()` | codec_spec.hpp | 26 | test_codecs | core |
| `make_lines_codec()` | codec_spec.hpp | 26 | test_codecs | core |
| `make_null_codec()` | codec_spec.hpp | 26 | test_codecs | core |
| `make_z32old_codec()` | codec_spec.hpp | 0 | **test_codecs** | core |
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
| `encoding_profile::set_*()` (20+ methods) | profile.cpp | 0 | **test_profile** | core |
| `encoding_profile::dither_string()` | profile.cpp | 0 | **test_profile** | core |
| `dithering_parameters::from_profile()` | dithering_parameters.cpp | 0 | **test_dithering_parameters** | core |

### SUBTITLE SUPPORT

| Function/Class | File | Test Count | Test Group | Level |
|----------------|------|------------|------------|-------|
| `read_timestamps()` | subtitles.cpp | 0 | **test_subtitles** | core |
| `next_subtitle()` | subtitles.cpp | 0 | **test_subtitles** | core |
| `read_subtitles()` | subtitles.cpp | 0 | **test_subtitles** | core |
| `subtitles_extract()` | subtitles.cpp | 0 | **test_subtitles** | core |
| `subtitle_burner::burn_into()` | subtitle_burner.hpp | 0 | **test_subtitles** | core |
| `watermark()` | watermark.cpp | 0 | **test_subtitles** | core |
| `burn_subtitle()` | watermark.cpp | 0 | **test_subtitles** | core |

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
| Utility | 26 | 2 | 24 | 8% |
| Core Data Structures | 65 | 25 | 40 | 38% |
| Codec/Compression | 15 | 10 | 5 | 67% |
| Config/Profiles | 9 | 6 | 3 | 67% |
| Subtitles | 7 | 0 | 7 | 0% |
| Orchestration | 13 | 0 | 13 | 0% |
| **TOTAL** | **135** | **43** | **92** | **32%** |

---

## Recommended New Test Groups

**Bold items** in the tables above indicate missing test coverage. Here are the recommended new test files:

### 1. test_string_utils.cpp (~10 tests)
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
8. Expand `test_codecs.cpp` - Add `z32old_codec`, `set_parameter()`

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

# Run specific test file (with doctest filters)
./unit_tests --test-case="*bitmap*"

# Verbose output
./unit_tests -s

# List all test cases
./unit_tests --list-test-cases
```

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
