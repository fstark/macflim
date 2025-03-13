# The following two lines are for compilation under homebrew on Mac M1
# -Wsign-compare -Wchar-subscripts -Wunused-but-set-variable -Wunused-variable 

CPP = g++-14

# -Werror 
CXXFLAGS += -Wall -Werror -Wno-deprecated-declarations -Wextra -I/opt/homebrew/include/ ${MORE}
LDLIBS += -L/opt/homebrew/lib/

all: ../flimmaker ../flimutil

# List of object files
OBJS = image.o reader.o writer.o ruler.o watermark.o imgcompress.o filesystem_reader.o ffmpeg_reader.o

# Pattern rule for compiling .cpp files to .o files
%.o: %.cpp
	${CPP} $(CXXFLAGS) -std=c++23 -c -O3 $< -o $@

# Specific rule for flimmaker.o with additional dependencies
flimmaker.o: flimmaker.cpp flimencoder.hpp flimcompressor.hpp compressor.hpp imgcompress.hpp framebuffer.hpp image.hpp ruler.hpp reader.hpp writer.hpp subtitles.hpp
	${CPP} $(CXXFLAGS) -std=c++23 -c -O3 -I liblzg/src/include flimmaker.cpp -o flimmaker.o

../flimmaker: flimmaker.o $(OBJS)
	${CPP} $(LDLIBS) -std=c++23 flimmaker.o $(OBJS) -lavformat -lavcodec -lavutil -o ../flimmaker

../flimutil: flimutil.c
	cc -O3 -Wno-unused-result flimutil.c -o ../flimutil

clean:
	rm -f ../flimmaker ../flimutil flimmaker.o $(OBJS)

debug: flimmaker.cpp flimutil.c imgcompress.cpp watermark.cpp image.cpp ruler.cpp flimencoder.hpp flimcompressor.hpp compressor.hpp imgcompress.hpp framebuffer.hpp image.hpp ruler.hpp
	${CPP} -O0 -std=c++2a -c -g -fsanitize=undefined imgcompress.cpp -o imgcompress.o
	${CPP} -Wall -O0 -std=c++2a -c -g -fsanitize=undefined flimmaker.cpp -o flimmaker.o
	${CPP} -O0 -std=c++2a -c -g -fsanitize=undefined watermark.cpp -o watermark.o
	${CPP} -O0 -std=c++2a -c -g -fsanitize=undefined image.cpp -o image.o
	${CPP} -O0 -std=c++2a -c -g -fsanitize=undefined ruler.cpp -o ruler.o
	${CPP} -O0 -std=c++2a -c -g -fsanitize=undefined reader.cpp -o reader.o
	${CPP} -O0 -std=c++2a -c -g -fsanitize=undefined writer.cpp -o writer.o
	${CPP} -O0 -std=c++2a -g -fsanitize=undefined imgcompress.o flimmaker.o watermark.o image.o ruler.o reader.o writer.o -lavformat -lavcodec -lavutil -o ../flimmaker
	cc -g -Wno-unused-result flimutil.c -o ../flimutil

video_test: video_test.c
	gcc -Wno-deprecated-declarations video_test.c -lavformat -lavcodec -lavutil -o video_test
