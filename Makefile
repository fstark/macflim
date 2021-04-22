all: flimmaker flimutil

imgcompress.o: imgcompress.cpp
	c++ -c -O3 imgcompress.cpp -o imgcompress.o

flimmaker.o: flimmaker.cpp
	c++ -c -O3 -I liblzg/src/include flimmaker.cpp -o flimmaker.o

flimmaker: flimmaker.o encode.o checksum.o imgcompress.o
	c++ flimmaker.o imgcompress.o encode.o checksum.o -o flimmaker

encode.o: liblzg/src/lib/encode.c
	c++ -c -O3 liblzg/src/lib/encode.c -o encode.o

checksum.o: liblzg/src/lib/checksum.c
	c++ -c -O3 liblzg/src/lib/checksum.c -o checksum.o

flimutil: flimutil.c
	cc -O3 -Wno-unused-result  flimutil.c -o flimutil

clean:
	rm -f flimmaker flimutil

debug: flimmaker.cpp flimutil.c
	c++ -g -fsanitize=undefined flimmaker.cpp -o flimmaker
	cc -g -Wno-unused-result flimutil.c -o flimutil
#	gdb ./flimmaker
