most other todos are in the .dsk file for the client

* Support non 512x342

* [DONE] Add sound
* [DONE] Unknown profile crashes (--profile se)
* [DONE] check issue with default duration parameter
* [DONE] codec number in encoding log
* [DONE] Track random ffmpeg crash
* [DONE] No need for argument to specify mandatory input
* [DONE] Add automatic gif generation
* [DONE] rename --dump into --mp4
* better options for .pgm image generation

* [DONE] Last sound frames
* [DONE] --from doesn't work for sound
* [DONE] Play last bits of flim on the mac
* [DONE] add --pgm for dumping pgms
* [DONE] command-line help

* [DONE] Add caching option for youtube

BUGS TO INVESTIGATE:

* rm 0.gif

---------------





### --codec **codec-definition**

The encoding is performed by a serie of codecs. Each frame is encoded by all the codecs with the existing byterate, and the one which produces the "best" image is choosen. As soon as the ``--codec`` command-line argument is specified, the codecs list is reset, and all codecs needs to be added manually. A codec is specified using its name, followed by an optional argument list, in the following format: ``--codec name:arg1=value1,arg2=value2``. The use of the ``--codec`` argument is mostly useful during development.

Each codec is defined by a name and an optional list of parameters. Here is the list of codecs and their parameters.

* null (0x00) : The ``null`` codec just doesn't encode anything. It is useful when an image doesn't change from a frame to the next.

* z16 (0x01) : The ``z16`` codec compresses the image in 16 pixels vertical bands. It is almost always inferior to the z32 codec.

* z32 (0x02) : The ``z32`` codec compresses the image in 32 pixels vertical bands. It is generally the most efficient codec.

* invert (0x03) : The ``invert`` codec (currently) bluntly inverts the whole image. This is useful in encoding movies that have sudden complete reversal of colors, which is a worst case for th ``z32`` codec, but can be trivialy encoded by ``invert``.

* lines (0x04) : The ``lines`` codec encodes a fixed amount of consecutive horizontal lines. It is usefull when there is a large change in the image, as it has less overhead than the z32 codec. The number of lines than can be encoded in a single frame is based on an average of 50 bytes per lines (which is wrong, but right)

The 0x00, 0x01, 0x02, 0x03 and 0x04 are the *signature* of the codecs, which can also be seen with the ``--watermark auto`` option.

One can see the exact behavior of each codec by specifying those one by one:

./flimmaker gangnam-style.mp4 --mp4 out.mp4 --duration 20 --codec z16


[to be continued]




Old stuff to be moved or deleted



creating the bullet-demo gif:

( sleep 1 ; xdotool type "ls -l matrix.mp4" && xdotool key Return && sleep 1 && xdotool type "./flimmaker matrix.mp4 --from 01:46:27 --duration 10 --mp4 bullet.mp4 --flim bullet.flim --profile se30" && xdotool key Return ; sleep 60 ) &
clear

using 'peek' as a screen grabber


ls -l matrix.mp4
./flimmaker --profile se30 matrix.mp4 --from 01:46:27 --duration 10 --mp4 bullet.mp4 --flim bullet.flim
vlc bullet.mp4
ls -l bullet.mp4
ls -l bullet.flim


note:

make && ./flimmaker --input badapple.mp4 --from 0 --duration 3600 --dump badapple-se30.mp4 --bars false --filters k20w20c && open badapple-se30.mp4




# SWEET DREAMS PLUS

# ordered
./flimmaker sweet-dreams.mp4 --profile plus --out sweet-dreams-plus-2.flim --mp4 sweet-dreams-plus-2.mp4 --bars none --filters Zzk10w20g1bbsc

./flimmaker sweet-dreams.mp4 --profile plus --bars false --filters Z24k8w8gbbsc --mp4 output3.mp4 &


# floyd
./flimmaker sweet-dreams.mp4 --profile plus --dither error --error-algorithm floyd --error-bleed 0.88 --error-bidi true --filters k10w10gbbsc --bars false --filters k1gbbsc --mp4 sweet-dreams-plus.mp4 --out sweet-dreams-plus.flim




./flimmaker sweet-dreams.mp4 --profile se --out sweet-dreams-se.flim --mp4 sweet-dreams-se.mp4 --bars none --filters Zzk10w20g1bbsc

./flimmaker gangnam-style.mp4 --profile se --out gangnam-style-se.flim --mp4 gangnam-style-se.mp4 --bars false --filters w10k10gsc

./flimmaker everybreath.mp4 --profile se --out everybreath-se.flim --mp4 everybreath-se.mp4 --bars false --filters w3k10gsc





./flimmaker sweet-dreams.mp4 --profile plus --from 55 --duration 5 --gif sweet-dreams-1.gif 
./flimmaker sweet-dreams.mp4 --profile plus --bars none --filters Zk10w20g1bbsc --from 55 --duration 5 --gif sweet-dreams-2.gif 
./flimmaker sweet-dreams.mp4 --profile plus --bars none --filters Zk10w20g1bbsc --dither floyd --from 55 --duration 5 --gif sweet-dreams-3.gif 
./flimmaker sweet-dreams.mp4 --profile se --bars none --filters Zk10w20g1bbsc --from 55 --duration 5 --gif sweet-dreams-4.gif 
./flimmaker sweet-dreams.mp4 --profile se30 --bars none --from 55 --duration 5 --gif sweet-dreams-5.gif 
./flimmaker sweet-dreams.mp4 --profile se30 --bars none --filters Zk10w20g1bbsc --from 55 --duration 5 --gif sweet-dreams-6.gif 

# Stability parameter demo

# .5 => We can see some leftover pixels around Annie face when she moves. However, there is no crawling pixels effect around
./flimmaker sweet-dreams.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.5 --from 52 --duration 5 --gif stability-1a.gif 
./flimmaker sweet-dreams.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.3 --from 52 --duration 5 --gif stability-1b.gif 
./flimmaker sweet-dreams.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.2 --from 52 --duration 5 --gif stability-1c.gif 
./flimmaker sweet-dreams.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.0 --from 52 --duration 5 --gif stability-1d.gif 

# When encoding for a plus, however, the stability helps using less bandwith, and the resulting image is closer to the source material, see Annie s hand. At low stability, the encoder gives up and use horizontal copies, but it doesn t help
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.5 --from 52 --duration 5 --gif stability-2a.gif 
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.3 --from 52 --duration 5 --gif stability-2b.gif 
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.2 --from 52 --duration 5 --gif stability-2c.gif 
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.0 --from 52 --duration 5 --gif stability-2d.gif 

# If we just look at only the stadard z32 encoding performance
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.5 --from 52 --duration 5 --gif stability-3a.gif --codec z32
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.3 --from 52 --duration 5 --gif stability-3b.gif --codec z32
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.2 --from 52 --duration 5 --gif stability-3c.gif --codec z32
./flimmaker sweet-dreams.mp4 --profile plus --dither floyd --bars none --filters Zk10w20g1bbsc --stability 0.0 --from 52 --duration 5 --gif stability-3d.gif --codec z32

# Compare the frame of the window, and the reflection of the drummer in the bottom 
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zg1bbsc --stability 0.5 --from 2:2 --duration 5 --gif stability-4a.gif 
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zg1bbsc --stability 0.3 --from 2:2 --duration 5 --gif stability-4b.gif 
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zg1bbsc --stability 0.2 --from 2:2 --duration 5 --gif stability-4c.gif 
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zg1bbsc --stability 0.0 --from 2:2 --duration 5 --gif stability-4d.gif 

./flimmaker everybreath.mp4 --profile plus --bars none --filters Zg1bbsc --stability 0.5 --from 2:2 --duration 5 --dither floyd --gif stability-5a.gif
./flimmaker everybreath.mp4 --profile plus --bars none --filters Zg1bbsc --stability 0.3 --from 2:2 --duration 5 --dither floyd --gif stability-5b.gif
./flimmaker everybreath.mp4 --profile plus --bars none --filters Zg1bbsc --stability 0.2 --from 2:2 --duration 5 --dither floyd --gif stability-5c.gif
./flimmaker everybreath.mp4 --profile plus --bars none --filters Zg1bbsc --stability 0.0 --from 2:2 --duration 5 --dither floyd --gif stability-5d.gif

./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.5 --from 2:2 --duration 5 --dither floyd --gif stability-6a.gif
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.3 --from 2:2 --duration 5 --dither floyd --gif stability-6b.gif
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.2 --from 2:2 --duration 5 --dither floyd --gif stability-6c.gif
./flimmaker everybreath.mp4 --profile perfect --bars none --filters Zk10w20g1bbsc --stability 0.0 --from 2:2 --duration 5 --dither floyd --gif stability-6d.gif

./flimmaker everybreath.mp4 --profile plus --bars none --filters Zk10w20g1bbsc --stability 0.5 --from 2:2 --duration 5 --dither floyd --gif stability-7a.gif
./flimmaker everybreath.mp4 --profile plus --bars none --filters Zk10w20g1bbsc --stability 0.3 --from 2:2 --duration 5 --dither floyd --gif stability-7b.gif
./flimmaker everybreath.mp4 --profile plus --bars none --filters Zk10w20g1bbsc --stability 0.2 --from 2:2 --duration 5 --dither floyd --gif stability-7c.gif
./flimmaker everybreath.mp4 --profile plus --bars none --filters Zk10w20g1bbsc --stability 0.0 --from 2:2 --duration 5 --dither floyd --gif stability-7d.gif


# Example with a lot of transitions

make && ./flimmaker sweet-dreams.mp4 --profile plus --bars false --from 1:27 --duration 5 --gif sweet-dreams.gif --filters Zk10w20g1bbsqc --dither floyd



make && ./flimmaker ipodad1.mp4 --out ipodad1.flim --dither ordered --filters w10k10bsq5c --mp4 out.mp4 --bars false 




# Generate asset

# macflim.gif
make && ./flimmaker ipodad1.mp4 --gif assets/macflim.gif --from 9.8 --duration 2 --profile perfect --filters g0.8q5c --dither ordered





make && ./flimmaker ipodad1.mp4 --profile plus --out ipodad1.flim --dither ordered --filters w10k10bsq5c --bars true



# Billie Jean plus
make && ./flimmaker billiejean.mp4 --profile plus --out billiejean-plus.flim --byterate 1500 --stability 0.5 --half-rate true --group false --bars true --dither floyd --filters w10k10g2bszzq --codec null --codec z32 --mp4 out.mp4
./flimmaker --cache /tmp/palmer.mp4 https://www.youtube.com/watch?v=UrGw_cOgwa8 --mp4 simply.mp4
