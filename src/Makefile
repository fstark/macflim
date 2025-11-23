# Set default parallel build option
MAKEFLAGS += -j16

# The following two lines were for compilation under homebrew on Mac M1
# They may or may not be still necessary
# -Wsign-compare -Wchar-subscripts -Wunused-but-set-variable -Wunused-variable 

# Compiler needs to support C++20
CPP = g++-14

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
CXXFLAGS += -Wall -Werror -Wno-deprecated-declarations -Wextra -std=c++2a

# Default compile target
all: release

debug: CXXFLAGS += -O0 -g -fsanitize=undefined
release: CXXFLAGS += -O3

debug: $(TARGET)
release: $(TARGET)

test: $(TARGET)
	cd ../tests && bash run_tests.sh $(UNAME_M)

# List of object files
OBJS = image.o reader.o writer.o ruler.o watermark.o imgcompress.o filesystem_reader.o ffmpeg_reader.o flimmaker.o common.o

# Include the dependency files
-include $(OBJS:.o=.d)

$(TARGET): $(OBJS)
	${CPP} ${CXXFLAGS} $(LDLIBS) $(OBJS) -lavformat -lavcodec -lavutil -o $(TARGET)

clean:
	rm -f ../flimmaker ../flimmaker.exe $(OBJS) $(OBJS:.o=.d)

# Pattern rule for compiling .cpp files to .o files
$(OBJS): %.o: %.cpp
	${CPP} $(CXXFLAGS) -MMD -MP -c $< -o $@
