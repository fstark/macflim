# Set default parallel build option
MAKEFLAGS += -j16

# The following two lines were for compilation under homebrew on Mac M1
# They may or may not be still necessary
# -Wsign-compare -Wchar-subscripts -Wunused-but-set-variable -Wunused-variable 

# Compiler needs to support C++23 generators
CPP = g++-14

# Default compilation flags
CXXFLAGS += -Wall -Werror -fsanitize=undefined -Wno-deprecated-declarations -Wextra -I/opt/homebrew/include/ -std=c++2a 

# Default compile target
all: debug

debug: CXXFLAGS += -O0 -g
release: CXXFLAGS += -O3 -Werror

debug: ../flimmaker ../flimutil
release: ../flimmaker ../flimutil

# List of object files
OBJS = image.o reader.o writer.o ruler.o watermark.o imgcompress.o filesystem_reader.o ffmpeg_reader.o flimmaker.o common.o

# Include the dependency files
-include $(OBJS:.o=.d)

../flimmaker: $(OBJS)
	${CPP} ${CXXFLAGS} $(LDLIBS) $(OBJS) -lavformat -lavcodec -lavutil -o ../flimmaker

../flimutil: flimutil.c
	${CPP} ${CXXFLAGS} flimutil.c -o ../flimutil

clean:
	rm -f ../flimmaker ../flimutil $(OBJS) $(OBJS:.o=.d)

# Pattern rule for compiling .cpp files to .o files
$(OBJS): %.o: %.cpp
	${CPP} $(CXXFLAGS) -MMD -MP -c $< -o $@
