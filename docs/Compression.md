In order to play sound, I need to gain space in files:

Compression is achived by storing the xor between consecutive frames, and compressing this data. Hopefully, it should be smaller, as the encoding try to keep the error diffusion pixels at the same place.

# Last try at describing the flim file format

Flim length is a multiple of 2, by adding an optional 0x00

4 Bytes: 'F', 'L', 'I', 'M'
1 Byte: 0x0d

1017 bytes: comment (used to store the encoding command, so ``head -2 x.flim`` gives the command used)

2 bytes: Flecter checksum of the rest of the file.

Rest of the file:

2 bytes: 0x1 == version 1
2 bytes: entries in the header
For each entry:
    2 bytes: type
    4 bytes: offset to data (starting just after this header)
    4 bytes: data size

Type of entries:
    0x00: movie info
    0x01: movie data
    0x02: table of content (more later)
    0x03: poster

Unknown entries should be ignored.

Movie info data (entry 0x00):
    2 bytes: width
    2 bytes: height
    2 bytes: 0 = with sound, 1 = silent
    4 bytes: number of frames (screen updates)
    4 bytes: number of ticks (60th of a second)


# New new format:

## Definitions

### Tick: A tick is a Macintosh tick of 1/60th of a second. 370 bytes of sound must be provided at every frame

### Frame definition

A frame is a single screen update. The frame rate is variable, as it is often not a sub-multiple of 60. Also, for compression purposes, some frame may be skipped.

### Block definition

A block is a set of 20 frames, asynchronously loaded from disk.

*It would be a good idea to get rid of this concept and store a directory of frame pointers in the flim header. This would enable run-time optimisation of read. Also, by adding a directory of pre-calulated frames, it would enable to seek withing flims*

*However, there still need to be such concept in the encoder to define the "lossy optimisation boudaries"*

## Concept

We do a single sound driver write and a single screen update per frame.

## Encoding is a succession of blocks, containing a list of frames.

    4 bytes block size [block_size]
    block_size bytes

Note: *Movie is supposed to start with a black screen*

# Frame encoding

A frame always encode a single sound buffer, followed by a single screen update buffer

    4 bytes data size
        2 bytes: ticks count == tick_counts
            6 bytes FFsynth header
            tick_counts * 370 bytes of sounds
        2 bytes: video encoding
            variable count of data bytes

# Video encoding

## 0x0000 : skip

no databytes. no screen updates

## 0x0001 : fulldata

21888 bytes of data, representing the full screen

## 0x0002 : bytexor

A stream of packbits-like bytes describing xor updates to the previous image

    1 - 127=n => xor the next n bytes with onscreen data
    -1 - -128=n => skip -n bytes
    0 => end of data


# Additional ideas of video encoding:

## Pre-pack skip + xor

    Encoding 3:
        n1 n2 [n2*2 bytes of data]
        => skip n1*2 bytes, xor the n2*2 next bytes of data
        [n1,n2] = [0,0] => end of data

## Word and long versions of xor encoding

## Packbits full screen for seeking purposes

# Lossy compression

Lossy compression is enabled by incresing the stability of the encoded image. A higher stability makes the image similar to the previous one.

The byterate of the resulting compressed image is computed as bytes/frames. There are 370 bytes of sound per ticks, so ``xor-diff / ticks + 370`` is the byterate.

The byterate can be lowered by:
* Increase stability
    increasing stability will make consecutive images more similar, and consume less bytes
* Increasing the tick
    as ticks are 2 or 3 of length, a frame on a 2 length tick can be extended to a 3 tick length, as to make more room for a sudden transition
    => if next frame tick is 3, make current frame 3 ticks.
* Interleave
    hard to encode image can be removed, and replaced by half-of the scanlines of the previous image and half of the scanlines of the next image. It will effectively disapear, but will be twice smaller

































4) SIZE
    4) 'FLIM'
    2) #frame 20
    2) filler2
        2) sound_mode
        4) fixed
        370) sound
        2) vid_size
        xx) video



