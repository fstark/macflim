all: ../flimmaker ../flimutil

image.o: image.cpp imgcompress.hpp image.hpp
	c++ -std=c++2a -c -O3 image.cpp -o image.o

reader.o: reader.cpp reader.hpp image.hpp
	c++ -std=c++2a -c -O3 reader.cpp -o reader.o

writer.o: writer.cpp writer.hpp image.hpp
	c++ -std=c++2a -c -O3 writer.cpp -o writer.o

ruler.o: ruler.cpp ruler.hpp
	c++ -std=c++2a -c -O3 ruler.cpp -o ruler.o

watermark.o: watermark.cpp imgcompress.hpp image.hpp
	c++ -std=c++2a -c -O3 -I liblzg/src/include watermark.cpp -o watermark.o

imgcompress.o: imgcompress.cpp imgcompress.hpp image.hpp
	c++ -std=c++2a -c -O3 imgcompress.cpp -o imgcompress.o

flimmaker.o: flimmaker.cpp flimencoder.hpp flimcompressor.hpp compressor.hpp imgcompress.hpp framebuffer.hpp image.hpp ruler.hpp reader.hpp writer.hpp
	c++ -std=c++2a -c -O3 -I liblzg/src/include flimmaker.cpp -o flimmaker.o

../flimmaker: flimmaker.o imgcompress.o image.o watermark.o ruler.o reader.o writer.o
	c++ -std=c++2a flimmaker.o imgcompress.o image.o watermark.o ruler.o reader.o writer.o -lavformat -lavcodec -lavutil -o ../flimmaker

../flimutil: flimutil.c
	cc -O3 -Wno-unused-result  flimutil.c -o ../flimutil

clean:
	rm -f ../flimmaker ../flimutil flimmaker.o imgcompress.o image.o watermark.o ruler.o reader.o writer.o

debug: flimmaker.cpp flimutil.c imgcompress.cpp watermark.cpp image.cpp ruler.cpp flimencoder.hpp flimcompressor.hpp compressor.hpp imgcompress.hpp framebuffer.hpp image.hpp ruler.hpp
	c++ -std=c++2a -c -g -fsanitize=undefined imgcompress.cpp -o imgcompress.o
	c++ -std=c++2a -c -g -fsanitize=undefined flimmaker.cpp -o flimmaker.o
	c++ -std=c++2a -c -g -fsanitize=undefined watermark.cpp -o watermark.o
	c++ -std=c++2a -c -g -fsanitize=undefined image.cpp -o image.o
	c++ -std=c++2a -c -g -fsanitize=undefined ruler.cpp -o ruler.o
	c++ -std=c++2a -c -g -fsanitize=undefined reader.cpp -o reader.o
	c++ -std=c++2a -c -g -fsanitize=undefined writer.cpp -o writer.o
	c++ -std=c++2a -g -fsanitize=undefined imgcompress.o flimmaker.o watermark.o image.o ruler.o reader.o writer.o -lavformat -lavcodec -lavutil -o ../flimmaker
	cc -g -Wno-unused-result flimutil.c -o ../flimutil
#	gdb ./flimmaker

video_test: video_test.c
	gcc -Wno-deprecated-declarations video_test.c -lavformat -lavcodec -lavutil -o video_test


#../flimmaker --input '/media/fred/Movies/Movies/Collections/Star Wars (1977-2019)/Star Wars - Episode V - The Empire Strikes Back (1980) [88]/Star Wars - Episode V - The Empire Strikes Back (1980) [720p,x264].mp4' --from 1:59:17.36 --duration 3.46 --bars false --group false --byterate 100000
