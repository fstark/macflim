# Set default parallel build option
MAKEFLAGS += -j16

# The following two lines are for compilation under homebrew on Mac M1
# -Wsign-compare -Wchar-subscripts -Wunused-but-set-variable -Wunused-variable 

CPP = g++-14

# -Werror 
CXXFLAGS += -Wall -Werror -Wno-deprecated-declarations -Wextra -I/opt/homebrew/include/ ${MORE}
LDLIBS += -L/opt/homebrew/lib/

all: ../flimmaker ../flimutil

# List of object files
OBJS = image.o reader.o writer.o ruler.o watermark.o imgcompress.o filesystem_reader.o ffmpeg_reader.o flimmaker.o 

# Pattern rule for compiling .cpp files to .o files and generating dependencies
%.o: %.cpp
	${CPP} $(CXXFLAGS) -std=c++23 -MMD -MP -c -O3 $< -o $@

# Include the dependency files
-include $(OBJS:.o=.d)

../flimmaker: $(OBJS)
	${CPP} $(LDLIBS) -std=c++23 $(OBJS) -lavformat -lavcodec -lavutil -o ../flimmaker

../flimutil: flimutil.c
	cc -O3 -Wno-unused-result flimutil.c -o ../flimutil

clean:
	rm -f ../flimmaker ../flimutil $(OBJS) $(OBJS:.o=.d)

# Debug target
debug: CXXFLAGS += -O0 -std=c++2a -g -fsanitize=undefined
debug: $(OBJS)
	${CPP} $(CXXFLAGS) $(OBJS) -lavformat -lavcodec -lavutil -o ../flimmaker
	cc -g -Wno-unused-result flimutil.c -o ../flimutil

# Pattern rule for compiling .cpp files to .o files in debug mode
$(OBJS): %.o: %.cpp
	${CPP} $(CXXFLAGS) -MMD -MP -c $< -o $@
