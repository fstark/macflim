all: flimmaker flimutil

flimmaker: flimmaker.cpp
	c++ -O3 flimmaker.cpp -o flimmaker

flimutil: flimutil.c
	cc -O3 -Wno-unused-result  flimutil.c -o flimutil

clean:
	rm -f flimmaker flimutil

debug: flimmaker.cpp flimutil.c
	c++ -g flimmaker.cpp -o flimmaker
	cc -g -Wno-unused-result flimutil.c -o flimutil
	gdb ./flimmaker
