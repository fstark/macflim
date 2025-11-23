# Set default parallel build option
MAKEFLAGS += -j16

# The following two lines were for compilation under homebrew on Mac M1
# They may or may not be still necessary
# -Wsign-compare -Wchar-subscripts -Wunused-but-set-variable -Wunused-variable 

# Compiler needs to support C++20
CPP = g++-14

# Detect Homebrew prefix (ARM: /opt/homebrew, Intel: /usr/local)
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
ifeq ($(UNAME_S),Darwin)
    BREW_PREFIX := $(shell brew --prefix 2>/dev/null || echo /usr/local)
    CXXFLAGS += -I$(BREW_PREFIX)/include/
    LDLIBS += -L$(BREW_PREFIX)/lib/ -Wl,-rpath,@executable_path/lib
else
    CXXFLAGS += -I/opt/homebrew/include/
endif

# Default compilation flags
CXXFLAGS += -Wall -Werror -Wno-deprecated-declarations -Wextra -std=c++2a

# Default compile target
all: release

debug: CXXFLAGS += -O0 -g -fsanitize=undefined
release: CXXFLAGS += -O3

debug: ../flimmaker
release: ../flimmaker

test: ../flimmaker
	cd ../tests && bash run_tests.sh $(UNAME_M)

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
