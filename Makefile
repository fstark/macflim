all: flimmaker flimutil

image.o: image.cpp imgcompress.hpp image.hpp
	c++ -c -O3 image.cpp -o image.o

watermark.o: watermark.cpp imgcompress.hpp image.hpp
	c++ -c -O3 -I liblzg/src/include watermark.cpp -o watermark.o

imgcompress.o: imgcompress.cpp imgcompress.hpp image.hpp
	c++ -c -O3 imgcompress.cpp -o imgcompress.o

flimmaker.o: flimmaker.cpp flimencoder.hpp flimcompressor.hpp compressor.hpp imgcompress.hpp framebuffer.hpp image.hpp 
	c++ -c -O3 -I liblzg/src/include flimmaker.cpp -o flimmaker.o

flimmaker: flimmaker.o imgcompress.o image.o watermark.o
	c++ flimmaker.o imgcompress.o image.o watermark.o -o flimmaker

flimutil: flimutil.c
	cc -O3 -Wno-unused-result  flimutil.c -o flimutil

clean:
	rm -f flimmaker flimutil flimmaker.o imgcompress.o image.o watermark.o

debug: flimmaker.cpp flimutil.c imgcompress.cpp
	c++ -c -g -fsanitize=undefined imgcompress.cpp -o imgcompress.o
	c++ -c -g -fsanitize=undefined flimmaker.cpp -o flimmaker.o
	c++ -g -fsanitize=undefined imgcompress.o flimmaker.o -o flimmaker
	cc -g -Wno-unused-result flimutil.c -o flimutil
#	gdb ./flimmaker
