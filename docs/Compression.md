# Flim file format

Compression stores xor deltas between consecutive frames, compressed via vertical strip or line-copy codecs. Dithering stability keeps pixels coherent across frames, reducing delta size.

## File layout

    1022 bytes: comment block
        bytes 0-4: 'F','L','I','M',0x0d (magic)
        bytes 5-1021: zero-padded string (encoding command, so ``head -2 x.flim`` works)
    2 bytes: Fletcher-16 checksum (big-endian) of everything after
    2 bytes: version (= 0x0001)
    2 bytes: component count
    For each component:
        2 bytes: type
        4 bytes: offset (from start of blob data area)
        4 bytes: size
    Blob data (concatenated component payloads)

Unknown component types should be ignored.

## Component types

### 0x00: info (16 bytes)

    2 bytes: width
    2 bytes: height
    2 bytes: silent (0 = sound, 1 = silent)
    4 bytes: frame count
    4 bytes: total ticks (1/60s units)
    2 bytes: byterate

### 0x01: movie

Frames serialized sequentially (see frame encoding below).

### 0x02: table of contents

Array of uint16, one per frame: byte size of that frame in the movie component. Enables seeking.

### 0x03: poster

Raw bitmap data, 128×86 pixels.

### 0x04: initial frame

    2 bytes: type tag (0x00)
    2 bytes: width
    2 bytes: height
    Raw bitmap bytes

Used to initialize the framebuffer before playback (for looping or seeking).

## Definitions

**Tick**: 1/60th of a second. 370 bytes of 8-bit unsigned audio per tick (≈22050 Hz).

**Frame**: A single screen update + its associated audio. Frame rate is variable.

## Frame encoding

Each frame in the movie component:

    2 bytes: ticks
    Sound block:
        2 bytes: sound_size
        If sound_size == 2: no audio
        If sound_size > 2 (= ticks * 370 + 8):
            2 bytes: ffMode (0)
            4 bytes: rate (65536)
            ticks * 370 bytes: audio samples
    Video block:
        2 bytes: video_size (includes this size field)
        video_size - 2 bytes: encoded video data

## Video encoding

All video data starts with a 4-byte header: ``0x00 0x00 0x00 <codec_signature>``, followed by codec-specific data.

### 0x00: null

No payload. No screen update.

### 0x01: z16 (vertical strips, 16-bit)

Column-major 16-bit words. Stream of runs:

    uint16 header: upper 8 bits = offset delta, lower 8 bits = word count
    count * 2 bytes: data (big-endian uint16 values)
    ...
    0x0000: terminator

Empty runs (count=0) bridge offset gaps > 255.

### 0x02: z32 (vertical strips, 32-bit)

Column-major 32-bit words. Stream of runs:

    uint32 header: upper 16 bits = count - 1, lower 16 bits = (offset + 1) * 4
    count * 4 bytes: data (big-endian uint32 values)
    ...
    0x00000000: terminator

### 0x03: invert

Inverts all bits on screen. No payload.

### 0x04: lines

Copies a contiguous block of lines from the target image:

    uint16: byte_count (line_count * bytes_per_row)
    uint16: byte_offset (start_line * bytes_per_row)
    byte_count bytes: raw pixel data

Selects the line range that maximizes pixel changes.

## Lossy compression

**Byterate**: bytes of video data per tick. Frame budget = byterate × ticks.

**Codec penalty**: effective budget = budget × penalty. z16 penalty = 0.45, others = 1.0.

**Codec selection**: all configured codecs encode each frame independently; the result with highest quality (bitmap proximity) wins.

**Vertical compressor budget**: screen positions are prioritized by perceptual delta (highest first). Positions are added until the budget is exhausted.

**Stability**: controls dithering hysteresis. The binarization threshold shifts based on the previous frame's pixel value. Higher stability → fewer pixel changes → less data to compress, at the cost of ghosting.



