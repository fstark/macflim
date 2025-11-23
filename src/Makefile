# Set default parallel build option
MAKEFLAGS += -j16

# The following two lines were for compilation under homebrew on Mac M1
# They may or may not be still necessary
# -Wsign-compare -Wchar-subscripts -Wunused-but-set-variable -Wunused-variable 

# Compiler: clang++ for macOS, g++-14 for Linux (for C++23 generators support)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CPP = clang++
else
    CPP = g++-14
endif

# Default compilation flags
CXXFLAGS += -Wall -Werror -Wno-deprecated-declarations -Wextra -I/opt/homebrew/include/ -std=c++2a 

# Default compile target
all: release

debug: CXXFLAGS += -O0 -g -fsanitize=undefined
release: CXXFLAGS += -O3

debug: ../flimmaker
release: ../flimmaker

# macOS universal binary (fat binary with both ARM64 and x86_64)
osx-fat: CPP = clang++ -arch arm64 -arch x86_64
osx-fat: CXXFLAGS += -O3
osx-fat: LDLIBS += -Wl,-rpath,@executable_path/lib
osx-fat: ../flimmaker

# List of object files
OBJS = image.o reader.o writer.o ruler.o watermark.o imgcompress.o filesystem_reader.o ffmpeg_reader.o flimmaker.o common.o

# Include the dependency files
-include $(OBJS:.o=.d)

../flimmaker: $(OBJS)
	${CPP} ${CXXFLAGS} $(LDLIBS) $(OBJS) -lavformat -lavcodec -lavutil -o ../flimmaker

clean:
	rm -f ../flimmaker $(OBJS) $(OBJS:.o=.d)

# Pattern rule for compiling .cpp files to .o files
$(OBJS): %.o: %.cpp
	${CPP} $(CXXFLAGS) -MMD -MP -c $< -o $@
