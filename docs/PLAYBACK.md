# MacFlim Player Architecture

This document describes the internal architecture of the MacFlim player (the `macsrc/` code that runs on classic Macintosh hardware). It covers the playback pipeline, the VBL and sound interrupt mechanisms, the codec/decoder layer, buffer management, and the FLIM file format as seen by the player.

---

## Overview

The player's job is to read a `.flim` file from disk, decompress video frames, and display them on the 1-bit Macintosh screen — optionally playing synchronized 8-bit mono audio. The fundamental challenge is that classic Macs are single-threaded with slow floppy I/O, so the design decouples **disk reading** (main loop, synchronous) from **frame display** (interrupt-driven, asynchronous).

### Key source files

| File | Role |
|------|------|
| `Playback.h` / `Playback.c` | Main playback orchestration, dual-buffer management, main loop |
| `Playback VBL.c` | VBL-driven silent playback (video only) |
| `Playback Sound.c` | Sound-driver callback playback (video + audio) |
| `Playback VBL Sound.c` | VBL-driven playback with direct DMA sound (Mac Plus and earlier) |
| `Playback Sync.c` | Synchronous reference playback (no interrupts, for debugging) |
| `Codec.h` / `Codec.c` | Video codec registry and decompression routines (C + 68K asm) |
| `Screen.h` / `Screen.c` | Screen abstraction, codec dispatch, on-screen logging |
| `Flim.h` / `Flim.c` | FLIM file format parsing, block/TOC management |
| `Buffer.h` / `Buffer.c` | Dual-buffer memory allocation |
| `Machine.h` / `Machine.c` | Hardware detection (CPU speed, ROM version, color support) |
| `Keyboard.h` / `Keyboard.c` | Keyboard polling for user commands (pause, skip, etc.) |

---

## The FLIM File Format (player's view)

### File header (bytes 0–1023)

```
Offset  Size   Content
0       5      Magic: 'FLIM' + 0x0A
5       1013   Comment string (human-readable, ignored by player)
1018    2      Fletcher16 checksum (covers bytes 1024 to EOF)
1020    4      Reserved
```

### Video header (starts at offset 1024)

```
Offset  Size   Content
1024    2      Version (must be 1)
1026    2      Stream count (up to 10)
1028    N×8    Stream descriptors
```

### Stream types

| ID | Name | Content |
|----|------|---------|
| 0 | Info | Width, height, frame count, silence flag, byterate, codec bitmap |
| 1 | Flim | Main interleaved video+audio frame data |
| 2 | TOC | Table of Contents — per-frame sizes (2 bytes each) |
| 3 | Poster | Thumbnail image (PICT) |
| 4 | InitialFrame | Optional initial framebuffer (applied before first decode) |

### Info stream (`FlimInfo`)

```c
struct FlimInfo {
    short width;           // Video width in pixels
    short height;          // Video height in pixels
    Boolean dummy;         // Reserved
    Boolean silent;        // TRUE if no audio track
    Size frameCount;       // Total number of frames
    Size ticks;            // Total tick count
    short byterate;        // KB/s (used for quality estimation)
    unsigned long codecs;  // Bitmap of codec IDs used in this file
};
```

### Frame data layout (repeating in the Flim stream)

Each group of frames in a block starts with a tick count, then alternates sound and video frame records:

```
2 bytes : tick count (VBL intervals before next group)

  Sound FrameDataRecord:
    2 bytes : data_size (total including header)
    6 bytes : sound header
    N bytes : PCM audio (8-bit unsigned mono, 22050 Hz)
              size is a multiple of 370 bytes (≈1 tick at 60 fps)

  Video FrameDataRecord:
    2 bytes : data_size (big-endian, includes these 2 bytes)
    N bytes : compressed video data

(repeat sound/video pairs for each frame in the group)
```

The `FrameDataPtr` type and `NextDataPtrS()`/`NextDataPtrV()` functions walk through these records sequentially in memory.

### Access table

At open time, the player reads the TOC stream and builds an **access table** (`AccessItem[]`): an array mapping block indices to `{frameCount, blockSize}` pairs. This tells the player how to partition the file into disk-read-sized blocks that fit in the available buffers.

---

## Dual-Buffer Architecture

The player allocates two equally-sized memory buffers at startup. While one buffer is being played by the interrupt handler, the main loop synchronously reads the next block of data from disk into the other buffer.

### Buffer allocation (`Buffer.c`)

```
available = MaxMem(&growBytes) + growBytes
usable    = available - leftBytes    // reserve space for TOC, offset tables, etc.
bufSize   = usable / 2               // split evenly
bufSize   = min(bufSize, maxSize)    // cap if needed
```

This works on machines with as little as 128 KB of RAM (Mac 128K: ~2 × 45 KB buffers).

### Block state machine

Each buffer (`BlockRecord`) tracks its lifecycle:

```
blockUnused (0)
    │  main loop calls FlimReadBlock()
    ▼
blockReading (1)
    │  FSRead() completes
    ▼
blockReady (2)
    │  interrupt handler picks it up
    ▼
blockPlaying (3)
    │  all frames consumed by interrupt handler
    ▼
blockPlayed (4)  ◄─── main loop can now reuse this buffer
    │
    ▼
blockClosed (5)  ◄─── signals "no more blocks coming" at end of file
```

The main loop never writes to a buffer that is `blockPlaying`. The interrupt handler never reads from a buffer that is `blockReading`. This state machine replaces semaphores (which don't exist on classic Mac OS).

### `BlockRecord` structure

```c
struct BlockRecord {
    short status;          // block state (see above)
    short index;           // which block of the file is loaded
    short ticks;           // current tick count for this frame group
    FrameDataPtr sound;    // pointer to current sound frame in buffer
    FrameDataPtr video;    // pointer to current video frame in buffer
    short frames_left;     // frames remaining in this block
    char buffer[1];        // actual data (variable length, sized by BufferInit)
};
```

---

## Main Playback Loop (`PlayFlimInternal`)

### 1. Initialization

1. Verify the screen is black-and-white (`MachineIsBlackAndWhite()`).
2. Prepare the screen for the flim's resolution (`ScreenVideoPrepare()`).
3. Apply the initial frame if present, or clear the video area to black.
4. Seek to the start of the flim data stream.
5. Initialize both block records (`FlimInitBlock()`).
6. Read block 0 synchronously into `gBlock1`.
7. Select a playback mode and install the interrupt handler (see below).
8. Set `gState = playingState`.

### 2. Steady-state loop

```
for index = 1 to blockCount-1:
    FlimReadBlock(flim, index, gReadBlock)   // sync disk read into free buffer
    gReadBlock = GetOtherBlock(gReadBlock)    // swap read target
    result = BlockWaitPlayed(gReadBlock)      // busy-wait for interrupt handler
                                              // to finish the previous block
    if result != kDone: abort/skip/restart
```

The main loop does nothing but read blocks and wait. All frame decoding happens in the interrupt handler.

### 3. Termination

1. Mark the unused block as `blockClosed` (signals end-of-file to interrupt handler).
2. Wait for the last block to finish playing.
3. Set `gState = stopRequestedState`.
4. Busy-wait until the interrupt handler acknowledges with `gState = stoppedState`.
5. Call `gPlayback.dispos()` to clean up.

### 4. Loop mode

For seamless looping, instead of terminating, the main loop seeks back to the start and reads block 0 into the free buffer, then jumps back to the steady-state loop.

---

## Playback Modes

The player selects a playback mode based on the file and the hardware:

```c
if (silent || user_pref_VBL || FlimGetIsSilent(flim))
    PlaybackVBLInit()          // VBL-only, no audio
else if (FlimGetIsSingleTick(flim) && MachineIsPlusOrEarlier())
    PlaybackVBLSoundInit()     // VBL + direct DMA sound
else
    PlaybackSoundInit()        // Sound driver callback
```

All three modes implement the same `struct Playback` interface:

```c
struct Playback {
    PlaybackControlFunc init;    // install interrupt handler, start in paused state
    PlaybackControlFunc resume;  // resume from pause
    PlaybackControlFunc dispos;  // clean up
};
```

### Mode 1: VBL-Only (`Playback VBL.c`)

**When used**: Silent flims, or when user explicitly selects VBL mode.

**Mechanism**: A VBL task is installed via `VInstall()`. The Macintosh calls it once per vertical blank (60 Hz NTSC / 50 Hz PAL).

**Interrupt handler** (`DoFrameSilent`):

1. Recover the A5 register (needed to access globals from interrupt context — the value is stashed in the 4 bytes before the VBL task record).
2. Check `gState`: if `stopRequestedState`, set `stoppedState` and return. If `pauseRequestedState`, set `pausedState` and return.
3. Set `vblCount = 1` (be called again next VBL).
4. Check for re-entry (`gInter > 1`): if the previous invocation hasn't finished, skip this frame and flash the screen as a visual warning.
5. If `frames_left == 0` on the current block, try to switch to the other block (if `blockReady`). If the other block is `blockClosed`, mark current as `blockPlayed` and stop. If neither is ready, wait (stall).
6. Decode one video frame: `ScreenUncompressFrame(gScreen, video->data)`.
7. Advance to the next frame: update `sound`, `video`, `ticks`, `frames_left`.
8. Set `vblCount = ticks` (skip VBLs if the frame should hold for multiple ticks).

**Note on tick counts**: A tick count > 1 means the frame should be displayed for multiple VBL intervals. The handler sets `vblCount` accordingly, so the system simply doesn't call it for that many VBLs.

### Mode 2: Sound Callback (`Playback Sound.c`)

**When used**: Flims with audio on Mac SE and later.

**Mechanism**: Uses the Macintosh Sound Manager. Writes a frame of audio (370 bytes) to device refnum -4 via `PBWrite(&pb, TRUE)` (async). The sound driver calls a completion callback (`DoFrame`) when it finishes playing the audio, creating a self-sustaining pump.

**Interrupt handler** (`DoFrame`):

1. Recover A5 from low-memory global `0x904` (CurrentA5).
2. Handle state transitions (stop, pause — during pause, play silence and keep the pump alive).
3. Check for re-entry; if so, play silence and return.
4. If `frames_left == 0`: switch blocks (same logic as VBL mode), or play silence if stalled.
5. Queue the next audio frame via `PBWrite()` (async). If muted, substitute the silence buffer.
6. **While the sound hardware plays audio**, decode the video frame: `ScreenUncompressFrame()`. This is the key trick — audio playback and video decompression overlap.
7. Advance to the next frame.

**Silence buffer**: Pre-allocated at init, contains 12 ticks worth of midpoint samples (value 128). Used during pause, stalls, muting, and when no sound data is present.

**Timing**: The sound driver controls timing. On the Macintosh, audio samples are clocked out once per horizontal scanline (342 visible lines + VBL), so a 370-byte audio frame is by definition exactly one video frame — the audio and video rates are locked to the same hardware timebase. The video decode must complete before the audio finishes playing, or the next callback will find the handler still busy (re-entry).

### Mode 3: VBL + Direct DMA Sound (`Playback VBL Sound.c`)

**When used**: Flims with audio on Mac Plus or earlier (68000-based systems).

**Why it exists**: The Sound Manager introduces audible cracking on 68000 Macs due to interrupt latency. This mode bypasses the sound driver entirely.

**Mechanism**: Same VBL task as Mode 1, but with audio handled by direct writes to the hardware sound DMA buffer.

**`CopyToSoundDMA()`**: Writes 370 audio bytes to the `SoundBase` low-memory global (address `0x266`). Each sample goes to the high-order byte of a 16-bit word in the DMA buffer:

```c
for (i = 0; i < 370; i++)
    soundBase[i * 2] = source[i];  // high-order byte only
```

**Constraint**: Requires single-tick frames (exactly 370 bytes of audio per frame). Multi-tick grouped frames cannot use this mode.

### Mode 4: Synchronous (`Playback Sync.c`)

**When used**: Enabled by `#define SYNCPLAY` at compile time.

**Mechanism**: No interrupt handlers at all. The main loop reads a block, then decodes and displays all its frames sequentially. Useful for codec debugging and performance measurement.

---

## Codec Layer

### Architecture

The codec layer is a dispatch table of function pointers. At screen preparation time, `ScreenVideoPrepare()` selects the appropriate decoder for each codec ID based on two factors:

- **same**: Is the flim resolution identical to the screen resolution? If so, use the direct "same" path (faster). Otherwise, use the offset-table "all" path.
- **ref**: Use the reference C implementation instead of the 68K assembly version? (For debugging.)

```c
DisplayProc CodecGetProc(eCodec codec, Boolean same, Boolean ref);
```

The result is stored in `ScreenRecord.procs[kCodecCount]`. Frame dispatch is then a simple indexed call:

```c
void ScreenUncompressFrame(ScreenPtr scrn, char *source) {
    eCodec codec = /* read from frame header */;
    scrn->procs[codec](source, &scrn->ccb);
}
```

### Codec registry

| ID | Name | Description |
|----|------|-------------|
| 0x00 | Null | No-op (placeholder frame, no data) |
| 0x01 | Z16 | Obsolete vertical codec |
| 0x02 | Z32 | Primary codec — vertical-strip compression |
| 0x03 | Invert | XOR all pixels with 0xFFFFFFFF (screen inversion) |
| 0x04 | Copy | Direct block copy of horizontal lines |

### Z32 Codec (the main codec)

Z32 is a delta-compression codec that exploits vertical locality. Each frame encodes only the pixels that changed since the previous frame, organized as vertical strips of 32-pixel-wide (4-byte) columns.

**On-disk format**:

```
Repeating chunks, terminated by 0x00000000:

  4-byte header:
    bits 31-16 : count - 1 (number of longs to write vertically)
    bits 15-0  : encoded T-offset
                   bits 15-2 = T[13:0]
                   bits 1-0  = T[15:14]
                 Decoded: T = (stored >> 2) | ((stored & 3) << 14)
                 Byte offset = (T - 1) * 4

  count × 4 bytes: pixel data (one long per vertical row)
```

**Decode algorithm (reference C)**:

```c
while (header = *source++) {
    stored = header & 0xFFFF;
    t_offset = (stored >> 2) | ((stored & 3) << 14);
    byte_offset = (t_offset - 1) * 4;
    dest = baseAddr + byte_offset;
    count = (header >> 16) + 1;

    while (count--) {
        *dest = *source++;
        dest += rowBytes;   // move down one scanline
    }
}
```

Each chunk copies `count` longs at vertical intervals of `rowBytes` bytes, starting at `byte_offset` from the screen base. This paints a vertical strip of changed pixels.

**T-offset encoding**: The 2-bit rotation trick (`bits 1-0 → bits 15-14`) extends the addressable range beyond 64 KB to 256 KB, supporting screens larger than 512×342 while remaining backward-compatible (for small screens, the low 2 bits are always 00).

**Assembly vs. reference**: The 68K assembly versions (`UnpackZ32_same`, `UnpackZ32_all`) use register-based loops and are significantly faster. The "same" variant addresses screen memory directly; the "all" variant goes through the offset table.

### Offset Table

When the flim resolution differs from the screen resolution, an offset table is pre-computed by `CreateOffsetTable()`. This is a flat array of pointers: for each 32-pixel position in the source video, the table stores the corresponding physical screen address. The Z32 "all" decoder indexes into this table instead of computing screen addresses directly.

```c
offsets[y * input_width_long + x] = base_addr + y * output_rowBytes + x * 4;
```

### Copy codec

Copies a contiguous run of horizontal scanlines:

```
2 bytes : byte count
2 bytes : destination offset from screen base
N bytes : raw pixel data
```

Used for the initial frame or full-screen updates where delta encoding would be larger.

### Invert codec

No data payload. XORs every pixel in the video area with 0xFFFFFFFF. Exists to handle flims that suddenly invert their entire content mid-stream — most notably "Bad Apple", which has a brutal black/white inversion in the middle. Without this codec, the encoder would have to delta-encode the full inversion, which would be enormous.

---

## The `CodecControlBlock`

Passed to every codec function, this structure describes the current playback geometry:

```c
struct CodecControlBlock {
    unsigned short source_width;      // flim width in pixels
    unsigned short source_width8;     // flim width in bytes
    unsigned short source_width32;    // flim width in longs (4-byte units)
    unsigned short source_height;     // flim height

    unsigned long **offsets32;        // offset table (NULL if same-size)

    unsigned short output_width8;     // screen row width in bytes
    unsigned short output_width32;    // screen row width in longs

    unsigned char *baseAddr;          // top-left of video area on screen
};
```

---

## Screen Management (`Screen.c`)

The `ScreenRecord` holds everything about the display target:

```c
struct ScreenRecord {
    unsigned char *physAddr;      // physical top-left of screen memory
    short width, height;          // physical screen dimensions
    short rowBytes;               // bytes per scanline

    short playback_left;          // horizontal offset for centering
    short playback_top;           // vertical offset for centering
    unsigned char *baseAddr;      // computed: physAddr + offsets

    Boolean ready;                // TRUE after ScreenVideoPrepare()

    short flim_width, flim_height;

    struct CodecControlBlock ccb;
    short stride4;                // (rowBytes - flim_width/8) / 4

    DisplayProc procs[kCodecCount];  // codec dispatch table
};
```

`ScreenVideoPrepare()` sets up the CCB, creates the offset table if needed, selects codec implementations, and validates that the screen can handle the flim's resolution.

`ScreenUncompressFrame()` dispatches to the appropriate codec function.

On-screen logging (`ScreenLog`, `ScreenLogString`) uses a built-in 8×8 bitmap font to render text directly into the framebuffer — safe to call from interrupt context for debug output.

---

## Hardware Detection (`Machine.c`)

The player adapts to different Macintosh models:

- **`MachineIsPlusOrEarlier()`**: Detects 68000-based systems (Mac 128K, 512K, Plus). These use direct DMA sound instead of the Sound Manager.
- **`MachineIsMinimal()`**: Detects 128K ROM or MacXL. Disables `MaxApplZone()` and `SysEnvirons()` (not available in 128K ROM). Uses `Environs()` instead.
- **`MachineIsBlackAndWhite()`**: Checks pixel depth via GDevice. Only 1-bit screens can play flims.
- **`bogoMips`**: A CPU benchmark computed at startup via a `TickCount()` loop. Example values: Mac Plus ≈ 2438, SE/30 ≈ 18450. Used for performance estimation.

---

## User Control During Playback

The main loop polls `CheckKeys()` during `BlockWaitPlayed()`:

| Key | Global flag | Effect |
|-----|-------------|--------|
| Escape | `gEscape` | Abort playback, return `kAbort` |
| → | `gSkip` | Skip to next file |
| ← | `gPrevious` | Go to previous file |
| R | `gRestart` | Replay current file |
| Space | `gPause` | Toggle pause |
| M | `gMuted` | Toggle audio mute |
| D | `gDebug` | Toggle debug overlay |

In the Mini Player, only the mouse button is checked (click = abort).

### Pause mechanics

1. `CheckKeys()` sets `gPause = TRUE`.
2. `BlockWaitPlayed()` detects this and sets `gState = pausedState`.
3. The interrupt handler sees `pausedState`: in sound mode, it plays silence to keep the pump alive; in VBL mode, it simply returns.
4. The main loop busy-waits in `BlockWaitPlayed()` until `gPause` is cleared.
5. On resume, `gPlayback.resume()` restarts the interrupt pump.

---

## Entry Points

### Full Player (`MacFlim Player.c`)

Full-featured standalone application. Initializes the Toolbox, detects hardware, offers file selection, library browsing, preferences. Supports all playback modes, pause, skip, loop, debug display.

### Mini Player (`Mini Player.c`)

Stripped-down player for resource-constrained machines. No library UI, no preferences. Plays a single file (selected via dialog or hardcoded). Target: machines with ≤64 KB available RAM.

### Self Player (`Self Player.c`)

Embeds the Mini Player into a FLIM file's resource fork, creating a self-executing FLIM. Uses the Macintosh dual-fork model: the data fork contains the FLIM, and the resource fork contains the player code.

### XCMD (`XCMD.c`)

HyperCard external command. Parses arguments (filename, window/fullscreen, loop, sound, mouse options) and plays a FLIM from within HyperCard.

---

## Architectural Patterns

### No dynamic allocation during playback

All memory is allocated upfront: the two block buffers, the silence buffer, the offset table. Nothing is allocated or freed during the playback loop. This prevents heap fragmentation and "out of memory" crashes mid-playback on machines with very limited RAM.

### Flag-based interrupt communication

The main loop and interrupt handler communicate exclusively through global flags (`gState`, `gPause`, `gEscape`, etc.) and the block state machine. There are no locks, semaphores, or message queues — classic Mac OS is single-threaded and non-preemptive, so atomic flag reads/writes are sufficient.

### Pluggable playback strategy

The `struct Playback` function-pointer interface lets the player select the interrupt mechanism at runtime without conditional checks inside the performance-critical interrupt handler.

### Overlapped I/O and decode

In sound callback mode, `PBWrite()` starts audio playback asynchronously, then the video frame is decoded *while the audio plays on the hardware*. This is the key performance trick: audio and video processing overlap.

### Codec dispatch via function pointer array

`procs[codec_id](source, &ccb)` avoids an if/else chain in the hot decode path. Adding a new codec is just adding an entry to the array.
