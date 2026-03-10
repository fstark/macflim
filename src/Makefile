# Set default parallel build option
MAKEFLAGS += -j16

# The following two lines were for compilation under homebrew on Mac M1
# They may or may not be still necessary
# -Wsign-compare -Wchar-subscripts -Wunused-but-set-variable -Wunused-variable 

# Compiler needs to support C++20
CPP = g++-14

# Extract version: VERSION file first (one dir up), then git describe, then fallback
VERSION ?= $(shell cat ../VERSION 2>/dev/null || git describe --tags --always --dirty 2>/dev/null || echo "dev-unknown")

# Detect operating system and architecture
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Platform-specific settings
ifeq ($(UNAME_S),Darwin)
    # macOS with Homebrew
    BREW_PREFIX := $(shell brew --prefix 2>/dev/null || echo /usr/local)
    CXXFLAGS += -I$(BREW_PREFIX)/include/
    LDLIBS += -L$(BREW_PREFIX)/lib/ -Wl,-rpath,@executable_path/lib
    TARGET := ../flimmaker
else ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    # Windows with MSYS2/MinGW
    TARGET := ../flimmaker.exe
else ifeq ($(findstring MSYS,$(UNAME_S)),MSYS)
    # Windows with MSYS2
    TARGET := ../flimmaker.exe
else
    # Linux or other Unix
    CXXFLAGS += -I/opt/homebrew/include/
    TARGET := ../flimmaker
endif

# Default compilation flags
CXXFLAGS += -Wall -Werror -Wno-deprecated-declarations -Wextra -std=c++2a -DVERSION=\"$(VERSION)\"

# Default compile target
all: release

.PHONY: all debug release test test-reference clean coverage coverage-clean format

debug: CXXFLAGS += -O0 -g -fsanitize=undefined
release: CXXFLAGS += -O3

debug: $(TARGET)
release: $(TARGET)

test: release unit_tests
	./unit_tests
	cd ../tests && bash run_tests.sh

test-reference: release
	cd ../tests && bash run_tests.sh --make-reference

# List of object files
OBJS = grayscale.o writer.o ruler.o watermark.o imgcompress.o filesystem_reader.o ffmpeg_reader.o flimmaker.o common.o flim.o subtitles.o flimutil.o dithering_parameters.o flimcompressor.o compressor_helper.o profile.o flimencoder.o cmdline.o

# Include the dependency files
-include $(OBJS:.o=.d)

$(TARGET): $(OBJS)
	${CPP} ${CXXFLAGS} $(LDLIBS) $(OBJS) -lavformat -lavcodec -lavutil -o $(TARGET)

# Unit test files
TEST_OBJS = test_main.o test_common.o test_imgcompress.o test_bitmap.o test_codecs.o \
            test_encoding_result.o test_frame.o test_profile.o test_ditherer.o \
            test_binary_io.o test_ruler.o test_string_utils.o test_timing.o test_arg_iterator.o \
            test_packzmap.o test_flim_file.o test_flim_errors.o test_grayscale_conversions.o test_grayscale_filters.o \
            test_dithering_parameters.o test_watermark.o test_subtitles.o test_grayscale_io.o \
            test_cmdline.o test_compressor_helper.o test_flimcompressor.o test_flimencoder.o \
            test_filesystem_reader.o test_stubs.o
TEST_SHARED_OBJS = common.o imgcompress.o ruler.o profile.o grayscale.o \
                   dithering_parameters.o watermark.o flim.o subtitles.o \
                   cmdline.o compressor_helper.o flimcompressor.o flimutil.o \
                   ffmpeg_reader.o filesystem_reader.o flimencoder.o writer.o

-include $(TEST_OBJS:.o=.d)

unit_tests: $(TEST_OBJS) $(TEST_SHARED_OBJS)
	${CPP} ${CXXFLAGS} $(LDLIBS) $(TEST_OBJS) $(TEST_SHARED_OBJS) -lavformat -lavcodec -lavutil -o unit_tests

# Coverage analysis
# Usage: make coverage
# Rebuilds tests with coverage instrumentation, runs them, and generates HTML report
coverage:
	@rm -f *.gcda *.gcno *.gcov coverage*.html coverage*.css
	@rm -f $(TEST_OBJS) $(TEST_SHARED_OBJS)
	$(MAKE) unit_tests CXXFLAGS="$(CXXFLAGS) --coverage"
	./unit_tests
	gcovr --gcov-executable gcov-14 --root . --filter='.*\.(cpp|hpp)$$' --exclude='test_.*' --html-details coverage.html --print-summary
	@echo ""
	@echo "Coverage report generated: coverage.html"

coverage-clean:
	rm -f *.gcda *.gcno *.gcov coverage*.html coverage*.css

clean: coverage-clean
	rm -f ../flimmaker ../flimmaker.exe $(OBJS) $(OBJS:.o=.d) unit_tests $(TEST_OBJS) $(TEST_OBJS:.o=.d)

format:
	clang-format -i *.cpp *.hpp

# Pattern rule for compiling .cpp files to .o files
$(OBJS) $(TEST_OBJS): %.o: %.cpp
	${CPP} $(CXXFLAGS) -MMD -MP -c $< -o $@
