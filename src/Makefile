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
TEST_OBJS = test/test_main.o test/test_common.o test/test_imgcompress.o test/test_bitmap.o test/test_codecs.o \
            test/test_encoding_result.o test/test_frame.o test/test_profile.o test/test_ditherer.o \
            test/test_binary_io.o test/test_ruler.o test/test_string_utils.o test/test_timing.o test/test_arg_iterator.o \
            test/test_packzmap.o test/test_flim_file.o test/test_flim_errors.o test/test_grayscale_conversions.o test/test_grayscale_filters.o \
            test/test_dithering_parameters.o test/test_watermark.o test/test_subtitles.o test/test_grayscale_io.o \
            test/test_cmdline.o test/test_compressor_helper.o test/test_flimcompressor.o test/test_flimencoder.o \
            test/test_filesystem_reader.o test/test_stubs.o
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
	@rm -f *.gcda *.gcno *.gcov coverage*.html coverage*.css test/*.gcda test/*.gcno test/*.gcov
	@rm -f $(TEST_OBJS) $(TEST_SHARED_OBJS)
	$(MAKE) unit_tests CXXFLAGS="$(CXXFLAGS) --coverage"
	./unit_tests
	gcovr --gcov-executable gcov-14 --root . --filter='.*\.(cpp|hpp)$$' --exclude='test/' --html-details coverage.html --print-summary
	@echo ""
	@echo "Coverage report generated: coverage.html"

coverage-clean:
	rm -f *.gcda *.gcno *.gcov coverage*.html coverage*.css test/*.gcda test/*.gcno test/*.gcov

clean: coverage-clean
	rm -f ../flimmaker ../flimmaker.exe $(OBJS) $(OBJS:.o=.d) unit_tests $(TEST_OBJS) $(TEST_OBJS:.o=.d) test/*.o test/*.d

format:
	clang-format -i *.cpp *.hpp test/*.cpp

# Pattern rule for compiling .cpp files to .o files
%.o: %.cpp
	${CPP} $(CXXFLAGS) -MMD -MP -c $< -o $@

# Pattern rule for compiling test .cpp files to .o files
# Add -I. to allow test files to include headers from parent directory
test/%.o: test/%.cpp
	${CPP} $(CXXFLAGS) -I. -MMD -MP -c $< -o $@
