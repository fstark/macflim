In order to play sound, I need to gain space in files:

* Sound will consume an additional 22200 per second
* Sound cannot be slowed down (currently, we slow down movies to 21fps)
* For any change at getting sound right, we need the load to be manageable.

Having flexible compression option will help 

Compression is achived by storing the xor between consecutive frames, and compressing this data. Hopefully, it should be smaller, as the encoding try to keep the error diffusion pixels at the same place.


    Compressed diff format:
    4 bytes:
        block size
    4 bytes:
        additional data size
    [additional data -- placeholder for sound
    6 bytes header
    570*FRAMECOUNT bytes
    ]
    2 bytes:
        framecount
    2 bytes*framecount
        size of frame data
        0 if frame is unchanged
    (framecount-1) encoded data
    The encoded data is the xor between consecutive frames.
    The first frame of a block is supposed to be compressing from a black screen (ie, it is the data itself)

New new format:

tick == 1/60th of a second
frame == single screen update

4 bytes block size

2 bytes frame count
    4 bytes framedata size
    2 bytes ticks count
    2 bytes sound size
        [sound data, including FFSynth header]
    2 byte frame encoding
        [frame data]

frame encoding:
    0 => no data
    1 => full image
    2 => byte-zero packed xor info
    3 => word-zero packed xor info
    4 => long-zero packed xor info


Encoding 1: W*X/8 bytes of data, representing the full image. Image may be smaller than 512x342

Encoding 2:
    1 - 127=n => xor the next n bytes with onscreen data
    -1 - -128=n => skip -n bytes
    0 => end of data

Encoding 3:
    

