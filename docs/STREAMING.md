# Streaming Architecture for MacFlim

## 1. Overview

This document describes the architecture for real-time video streaming from a Linux host to a vintage Macintosh SE/30 over Ethernet/UDP. The Linux host encodes video on the fly and sends compressed frame deltas as individual UDP packets. The Mac client decodes and displays them, reporting back which frames it successfully displayed and which it missed. The server uses this feedback to maintain a pixel-perfect simulation of the client's screen, ensuring that every encoded delta is maximally efficient.

**Key constraints:**
- Target: Mac SE/30 with Ethernet card, running MacTCP
- Transport: UDP over Ethernet — one frame per packet, no fragmentation
- Encoding: `group=false` (one tick per frame, 60 packets/second)
- Sound: deferred — focus on video first; if video works, sound is added to the frame payload later
- This document covers the **server-side architecture** (Linux encoder + protocol). The Mac player is out of scope but the protocol is designed with its constraints in mind (limited RAM, no dynamic allocation during playback).

### Why one frame = one packet?

At `group=false` with 60 fps, the per-frame video budget for an Ethernet-limited SE/30 is moderate — likely 1000–2000 bytes per frame, well under the 1500-byte Ethernet MTU. This eliminates fragmentation/reassembly complexity. When the entire scene changes, updates spread across multiple frames producing a "progressive reveal" effect, which is acceptable and even visually interesting for this use case.

---

## 2. Current Encoder Architecture

The existing encoding pipeline (all in `namespace macflim`):

```
input_reader → flimencoder → flimcompressor → compressor_helper → flim file
                                                     ↓
                                               Ditherer + compressor(s) + encoding_result
```

### The critical inner loop

The heart of encoding is in `compressor_helper::add()` ([src/compressor_helper.cpp](../src/compressor_helper.cpp)). For each input grayscale frame:

1. **Dither** to 1-bit via `Ditherer::dither()` → produces a `bitmap` target
2. **Compute budget**: `byterate × ticks` bytes available for video
3. **Try all codecs** in parallel: each creates an `encoding_result` that copies `current_fb_`, calls `compress(copy, target, budget)`, measures quality
4. **Pick best** result by `bitmap::proximity()`
5. **Update** `current_fb_` ← result bitmap (the new screen state)
6. **Accumulate** encoded frame into `frames_` vector

The key state is `current_fb_` — a `bitmap` representing what the encoder believes is currently on the Mac screen. Every delta is computed as `current_fb` → `target`. After encoding, `current_fb_` is updated to reflect what was *actually drawn* (which may be partial if budget was tight). In the batch pipeline this is correct: every encoded frame will be decoded in sequence, so `current_fb_` always matches the Mac's actual screen.

In streaming, this assumption breaks — packets can be lost, so what the Mac actually has on screen can diverge from what the server last encoded. Handling this divergence is the core challenge of the streaming architecture (see Section 6).

### What's reusable for streaming

| Component | Reusable? | Notes |
|---|---|---|
| `bitmap` | **Yes** | Core data type. Copyable (deep copy ~22KB). |
| `compressor` subclasses | **Yes** | Stateless per-frame. `compress(current, target, budget)` works with any `current`. |
| `encoding_result` | **Yes** | Self-contained per-frame encoding attempt. |
| `codec_spec` / factory | **Yes** | Stateless codec creation via `make_codec()`. |
| `Ditherer` | **Yes** | Stateful across frames (temporal stability), but only on the encoding side. |
| `encoding_profile` | **Partially** | Good for initial config. Byterate needs to be dynamic for streaming. |
| `frame::serialize()` | **Yes** | Already a good wire format for individual frames. |
| `compressor_helper` | **Adapt** | Core loop is reusable but needs to be split: extract a pure single-frame encode function, separate from accumulation/audio/timing. |
| `flimcompressor` | **Skip** | Thin batch orchestrator. Streaming has its own loop. |
| `flimencoder` / `flim` | **Skip** | File-format assembly, not needed for streaming. |

### What's missing

- **~~No standalone decoder.~~** Done — see [src/decoder.hpp](../src/decoder.hpp) / [src/decoder.cpp](../src/decoder.cpp). The encoder's inline decompression in `compressor.hpp` now calls `apply_delta()` too.
- **No network I/O** in the encoder codebase.
- **No feedback loop** — encoding is currently fire-and-forget.

---

## 3. Streaming Object Model

All new classes live in `namespace macflim`, use `snake_case` naming per project convention.

### Class diagram

```
streaming_session
├── input_reader              (existing — reads video frames)
├── Ditherer                  (existing — grayscale → 1-bit)
├── streaming_encoder         (new — wraps encode_frame())
├── client_state_tracker      (new — simulates client screen) ✓
├── adaptive_rate_controller  (new — adjusts byterate)
└── transport                 (new — UDP send/receive)
```

### `streaming_session`

Top-level object managing one streaming connection. Owns all components. Runs the main 60 Hz loop (Section 5). Handles session lifecycle: handshake → stream → teardown.

```cpp
class streaming_session
{
    std::unique_ptr<input_reader> reader_;
    Ditherer ditherer_;
    streaming_encoder encoder_;
    client_state_tracker tracker_;
    adaptive_rate_controller rate_ctrl_;
    std::unique_ptr<transport> transport_;
    encoding_profile profile_;
    uint32_t seq_ = 0;
};
```

### `streaming_encoder`

Thin wrapper around the extracted `encode_frame()` function (see Section 8). Takes a simulated client screen + dithered target + codecs + budget, returns the best encoding result.

```cpp
class streaming_encoder
{
    std::vector<codec_spec> codecs_;

public:
    // Encodes one frame: finds best codec within budget.
    // Mutates client_fb to reflect what was actually drawn.
    encoding_result encode(bitmap &client_fb,
                           const bitmap &target,
                           size_t budget);
};
```

The implementation is essentially lines 48–66 of the current [compressor_helper.cpp](../src/compressor_helper.cpp), extracted into a reusable function.

### `client_state_tracker`

Maintains a pixel-perfect simulation of the Mac's screen. This is the most architecturally important new class — and the most subtle.

```cpp
class client_state_tracker
{
    bitmap simulated_fb_;              // Last confirmed client screen state
    uint32_t simulated_seq_ = 0;      // Seq of last confirmed state
    
    struct in_flight_frame {
        uint32_t seq;
        std::vector<uint8_t> delta;    // Encoded delta bytes (compact, ~1-2KB each)
    };
    
    std::deque<in_flight_frame> in_flight_;  // Ordered ring buffer
    uint32_t last_acked_seq_ = 0;

public:
    // The screen state to encode against.
    // Computes: simulated_fb_ with all in-flight deltas applied optimistically.
    bitmap current_client_screen() const;
    
    // Record a frame we just sent
    void record_sent(uint32_t seq, std::vector<uint8_t> delta);
    
    // Process client feedback — replays deltas selectively to reconstruct true screen
    void process_feedback(uint32_t last_displayed_seq,
                          const std::vector<uint8_t> &history_bitmap);
};
```

**How it works:**

- **`current_client_screen()`** returns the server's best guess of what the client screen looks like *right now*: it starts from `simulated_fb_` (last confirmed state) and applies all in-flight deltas in order. This is what we encode against — equivalent to `current_fb_` in the batch pipeline.
- **`record_sent()`** appends `{seq, delta}` to the in-flight buffer. Only the delta bytes are stored (~1–2KB each), not full bitmaps.
- **`process_feedback()`** is where the real work happens. When the client reports which frames it displayed and which it missed, the tracker replays deltas from `simulated_fb_` forward — applying deltas for received frames, skipping missed ones — to arrive at the client's true screen state. This becomes the new `simulated_fb_`.
- Frames already in-flight when a miss is discovered were encoded against a *wrong* assumption. Their deltas still get applied by the client (to the wrong base), producing deterministically wrong visuals. The server replays the same sequence and arrives at the same wrong bitmap — so it still knows exactly what the client has. Subsequent frames naturally correct the damage (see Section 6 for a worked example).

### `adaptive_rate_controller`

Adjusts the per-frame byte budget based on packet loss feedback.

```cpp
class adaptive_rate_controller
{
    size_t max_byterate_;          // From profile (e.g. 6000 for SE/30)
    size_t current_byterate_;      // Current effective byterate
    
    // Sliding window of recent frame outcomes
    static constexpr size_t WINDOW = 60;  // 1 second at 60fps
    std::deque<bool> outcomes_;            // true=acked, false=missed

public:
    size_t budget_for_next_frame() const;
    void record_ack(uint32_t seq);
    void record_miss(uint32_t seq);
};
```

**Strategy (AIMD — additive increase, multiplicative decrease):**
- Loss ratio > 10% over the window → reduce `current_byterate_` by 20%
- Loss ratio = 0 for 2 full windows (120 frames) → increase by 5%, capped at `max_byterate_`
- Minimum byterate = 8 bytes/frame (enough for a null codec header)

This is deliberately simple. We can tune thresholds later based on real-world testing.

### `transport` (abstract)

```cpp
class transport
{
public:
    virtual ~transport() = default;
    
    // Send a frame packet to the client
    virtual void send_frame(uint32_t seq, uint16_t ticks,
                            const std::vector<uint8_t> &video_data) = 0;
    
    // Send session setup response
    virtual void send_hello(const encoding_profile &profile) = 0;
    
    // Non-blocking receive of any pending client messages
    // Returns empty vector if nothing available
    virtual std::vector<client_message> receive() = 0;
};
```

Abstracted behind an interface so we can later support serial/LocalTalk transports. The initial implementation is `udp_transport` using POSIX sockets with non-blocking I/O.

### `frame_decoder` — DONE

Implemented in [src/decoder.hpp](../src/decoder.hpp) / [src/decoder.cpp](../src/decoder.cpp). Two `apply_delta()` overloads (with/without 4-byte codec header). The encoder's inline decompression in `compressor.hpp` now calls `apply_delta()` too. Verified bit-perfect via round-trip tests in [src/test/test_decoder.cpp](../src/test/test_decoder.cpp).

---

## 4. Protocol Design

### Session handshake (client-initiated)

The client connects and tells the server what delivery format it wants. This is fundamentally an `encoding_profile` — the client is not saying "I am an SE/30" but "I'd like content delivered in this format". Profile names are just shortcuts.

**Client → Server: HELLO**
```
[4 bytes] magic: "FLMS" (FLiM Stream)
[2 bytes] protocol version (1)
[2 bytes] requested width
[2 bytes] requested height
[2 bytes] requested byterate
[1 byte]  requested dither mode (0=ordered, 1=error, 2=blue)
[1 byte]  num codec signatures
[N bytes] codec signatures (one byte each)
[1 byte]  profile name length (0 if no shortcut)
[N bytes] profile name (e.g. "se30") — if present, overrides individual fields
```

**Server → Client: HELLO_ACK**
```
[4 bytes] magic: "FLMA"
[2 bytes] protocol version (1)
[2 bytes] actual width
[2 bytes] actual height
[2 bytes] initial byterate
[2 bytes] num codec signatures
[N bytes] codec signatures
```

The server may adjust parameters (e.g. clamp byterate to what the source can sustain). The ACK tells the client the actual parameters in use.

### Frame packet (Server → Client)

```
[4 bytes] magic: "FLMF"
[4 bytes] sequence number (monotonically increasing)
[2 bytes] ticks (always 1 for group=false)
[N bytes] encoded video data (codec header + delta, same format as frame::serialize() video block)
```

Total overhead: 10 bytes per frame. With ~1000–2000 bytes of video data, packets are 1010–2010 bytes — comfortably under 1500-byte Ethernet MTU for typical content.

**Note on sound:** When sound is added later, the frame packet gains a sound block after ticks, using the same format as `frame::serialize()` (2 bytes size + 370 bytes per tick of audio data). The protocol version would be bumped.

### Feedback packet (Client → Server)

```
[4 bytes] magic: "FLMR" (Report)
[4 bytes] last_displayed_seq (highest sequence number processed by client)
[2 bytes] N (number of history bits, e.g. 128)
[N/8 bytes] history bitmap
```

The history bitmap covers the last N frames ending at `last_displayed_seq`. Bit 0 (LSB of the first byte) corresponds to `last_displayed_seq`, bit 1 to `last_displayed_seq - 1`, and so on. A `1` bit means the frame was successfully displayed; a `0` means it was missed (not received or not decoded in time).

With N=128 (2 seconds at 60 fps), the bitmap is 16 bytes — making the total feedback packet 22 bytes. This is always the same size regardless of how many frames were missed.

**Why a bitmap instead of a list of missed sequences?**

- **Idempotent.** Each feedback packet is self-contained. If the server misses a feedback packet (UDP is unreliable in both directions), the next one still carries the full picture. With a list-based approach, a lost feedback packet creates a blind spot — the server would never learn about missed frames reported only in the lost packet.
- **Fixed size.** 16 bytes is tiny and predictable. A list of missed sequences (4 bytes each) is more compact when misses are very rare (0–3 misses) but grows unboundedly in bad conditions.
- **Simple on the Mac.** The client maintains a shift register: shift left on each frame tick, set bit 0 if the frame was displayed. Send the register as-is.

The server only needs to look at the bits corresponding to the range `(simulated_seq_, last_displayed_seq]` — i.e., frames newer than its last confirmed state. Older bits are irrelevant (already confirmed).

### Timing

- Server sends at 60 Hz, paced to real-time
- Client feedback at up to 60 Hz (every frame), but the server must tolerate gaps
- If the server gets no feedback for N/60 seconds (~2s for N=128), communication is considered broken — pause encoding and attempt reconnection

---

## 5. Server Main Loop

```
streaming_session::run():
    // Handshake
    client_hello = transport.receive_hello()
    profile = resolve_profile(client_hello)
    transport.send_hello_ack(profile)
    
    // Initialize
    encoder.init(profile)
    tracker.init(bitmap::blank(profile.width, profile.height))
    rate_ctrl.init(profile.byterate)
    
    // Stream
    real_time_clock.start()
    
    while frame = reader.next():
        // Pace to real-time (wait if encoding is faster than playback)
        real_time_clock.wait_until(seq / 60.0)
        
        // Prepare target
        target = ditherer.dither(frame)
        
        // Encode against best-guess client screen
        client_fb = tracker.current_client_screen()  // simulated_fb_ + in-flight deltas
        budget = rate_ctrl.budget_for_next_frame()
        result = encoder.encode(client_fb, target, budget)
        
        // Send
        delta = result.get_video_encoded_data()
        transport.send_frame(seq, 1, delta)
        
        // Record delta (not full bitmap — compact storage)
        tracker.record_sent(seq, delta)
        seq++
        
        // Process feedback (non-blocking)
        for msg in transport.receive():
            tracker.process_feedback(msg.last_displayed, msg.history_bitmap)
            rate_ctrl.update(msg)
```

### Lookahead buffer

The server encodes 5–10 frames ahead of real-time playback. This provides a buffer against encoding jitter (some frames take longer to encode than others). The sequence number pacing ensures frames are sent at the correct time regardless of when they were encoded.

---

## 6. Client State Tracking — Deep Dive

This is the most subtle part of the architecture. The server must know *exactly* what the client's screen looks like, because the efficiency of delta encoding depends entirely on the accuracy of the "current" bitmap.

### Two levels of state

1. **`simulated_fb_`** — the last *confirmed* client screen state. Updated only when feedback arrives. This is ground truth.

2. **`current_client_screen()`** — `simulated_fb_` plus all in-flight deltas applied optimistically. This is what we encode against. It represents our best guess of what the client screen will look like by the time it processes the next frame.

### Why store deltas, not result bitmaps?

A result bitmap records "what the screen looks like if this frame *and all prior frames* were applied correctly." But when frames are missed, prior frames may *not* have been applied. The result bitmap becomes meaningless.

Deltas, on the other hand, are the raw encoded bytes — the actual XOR patches the client applies. By replaying them selectively (skipping missed ones), the server can reconstruct exactly what the client did, even in the presence of packet loss. And they're compact: ~1–2KB each vs ~22KB for a full bitmap.

### The in-flight problem

When we encode frame 10, frames 5–9 may still be in flight. We encode assuming they all arrived (optimistic). If frame 7 is later reported missed, the deltas for frames 8 and 9 were applied by the client to the *wrong base* (one without frame 7). This means:

- Frames 8 and 9 produce **wrong visual results** on the client (the XOR deltas hit wrong pixels)
- But the results are **deterministic** — the server can replay the same sequence and arrive at the same wrong bitmap
- Frame 10 onwards will **naturally correct** the damage, because the server now knows the true client state

### State transitions — worked example

```
Server state:
  simulated_fb_ = [after frame 4]     (last confirmed via feedback)
  in_flight_ = [F5, F6, F7, F8, F9]   (sent, awaiting feedback)

Encoding frame 10:
  current_client_screen() computes:
    start with simulated_fb_
    apply F5 delta → state_5
    apply F6 delta → state_6
    apply F7 delta → state_7
    apply F8 delta → state_8
    apply F9 delta → state_9
  encode F10 as: state_9 → target_10

Feedback arrives: last_displayed=9, missed=[7]
  process_feedback() replays:
    start with simulated_fb_ (after frame 4)
    apply F5 delta → ok
    apply F6 delta → ok
    skip F7        → (missed — screen unchanged)
    apply F8 delta → wrong base! (no F7) — but deterministically wrong
    apply F9 delta → also wrong-ish — but deterministically so
  simulated_fb_ = result of this replay (exact client screen)
  in_flight_ = []  (all entries ≤ 9 removed)

Encoding frame 11:
  current_client_screen() = simulated_fb_ (no in-flight frames)
  This accurately reflects the client's actual screen
  F11's delta will naturally fix the visual artifacts from the missed F7
```

### Protocol semantics

- `last_displayed_seq`: the highest sequence number the client has *processed* (attempted to display). This is the client's "clock" — it tells the server how far the client has progressed through the stream.
- `history bitmap`: bit N=0 is `last_displayed_seq`, bit N=1 is `last_displayed_seq - 1`, etc. A `1` bit means displayed successfully, `0` means missed.

So if `last_displayed_seq=7` and the bitmap starts with `0b11010111` (reading bits 0–7), that means:
- Seq 7: bit 0 = 1 → displayed
- Seq 6: bit 1 = 1 → displayed
- Seq 5: bit 2 = 0 → missed
- Seq 4: bit 3 = 1 → displayed
- Seq 3: bit 4 = 0 → missed
- Seq 2: bit 5 = 1 → displayed
- Seq 1: bit 6 = 1 → displayed
- Seq 0: bit 7 = 1 → displayed

The server reads the bits for the range it cares about (since `simulated_seq_`) and replays accordingly.

### Self-correcting property

The streaming encoder *doesn't retry individual frames*. It just always encodes "here's how to get from what you have now to what you should see now." If "what you have now" is damaged due to missed frames, the delta will include more data (requiring more budget), which is exactly the right behavior. Over the next few frames, the visual artifacts are progressively corrected — no special repair mechanism needed.

### Memory and compute cost

Each in-flight frame stores only its **delta bytes** (typically 1–2 KB). With a window of ~10 in-flight frames, storage is ~10–20 KB total — negligible.

`current_client_screen()` replays all in-flight deltas onto a copy of `simulated_fb_`. With ~10 frames, that's 10 × `apply_delta()` calls on a ~22KB bitmap — fast.

### What if feedback is very delayed?

If the server gets no feedback for 60+ frames, the in-flight buffer grows. The `current_client_screen()` computation becomes more expensive (more deltas to replay) and the risk of compounding errors from undetected misses increases. Mitigations:

- The history bitmap covers N=128 frames (~2 seconds). If the server gets no feedback for that duration, communication is considered broken — stop encoding.
- The rate controller will detect the lack of feedback and reduce byterate defensively before that point.
- In practice, with 60 Hz feedback, the in-flight window should be 2–5 frames (network round-trip). The 128-frame bitmap provides ample margin for occasional feedback packet loss.

---

## 7. Adaptive Rate Control

### Why it matters

Ethernet to an SE/30 has a theoretical bandwidth around 1.2 MB/s (10 Mbit minus overhead and Mac-side processing limits). But the actual sustainable rate depends on the Mac's ability to receive packets, decode them, and update the screen — all at 60 Hz. If we send too much data, the Mac can't keep up and starts dropping frames.

### Algorithm

```
window_size = 60  // 1 second
loss_ratio = missed_in_window / window_size

if loss_ratio > 0.10:
    current_byterate *= 0.80   // multiplicative decrease
    
elif loss_ratio == 0 for 120 consecutive frames:
    current_byterate = min(current_byterate * 1.05, max_byterate)  // additive increase

// Clamp
current_byterate = clamp(current_byterate, MIN_BYTERATE, max_byterate)
```

Where `MIN_BYTERATE = 8` (enough for a null codec 4-byte header per frame) and `max_byterate` comes from the profile.

### Considerations

- The initial byterate should be conservative (e.g. 50% of profile max) to avoid an initial burst of losses while the system stabilizes.
- More sophisticated algorithms (e.g. measuring round-trip time from send to ACK, or tracking the "headroom" between budget and actual encoded size) can be added later.
- The rate controller outputs a `budget` (bytes) not a `byterate` — since `group=false` and `ticks=1`, these are the same value. If grouping is ever re-enabled, budget = byterate × ticks.

---

## 8. Required Refactoring (Phase 1 — Do Now)

These changes prepare the codebase for streaming without implementing any streaming code. They also improve the existing batch pipeline.

### 8.1. Extract `encode_frame()` free function — DONE

**Goal:** Make the core encode-one-frame logic callable independently of `compressor_helper`'s batch state (audio, tick counter, frame accumulation).

**Done.** Extracted into [src/encode_frame.hpp](../src/encode_frame.hpp) / [src/encode_frame.cpp](../src/encode_frame.cpp). `compressor_helper::add()` now calls `encode_frame()` internally — zero behavior change, all existing tests pass. Unit tests in [src/test/test_encode_frame.cpp](../src/test/test_encode_frame.cpp) (8 test cases).

**Known issue found:** The `lines` codec has a pre-existing infinite loop when `budget < get_bytes_width()` (64 bytes) — `target_count` becomes 0 and the loop step is 0. Not introduced by this refactoring.

### 8.2. Extract frame delta decoder — DONE

**Implementation:** [src/decoder.hpp](../src/decoder.hpp) / [src/decoder.cpp](../src/decoder.cpp) — two `apply_delta()` overloads (with/without 4-byte codec header). Decodes all five codecs including vertical-packing conversion. Throws `std::runtime_error` for unknown signatures or truncated data. The inline decompression block in `compressor.hpp` (`#### Decompresses`) has been replaced with a call to `apply_delta()`.

Reference implementations: [macsrc/Codec.c](../macsrc/Codec.c) (68K C/asm: `UnpackZ32_same`, `UnpackZ32_all`, etc.).

Codec wire formats:

| Signature | Name | Wire format |
|---|---|---|
| 0x00 | null | No data. Screen unchanged. |
| 0x01 | z16 | `[uint16 header: skip<<8 \| count] [count × uint16 data]...` terminated by `0x0000` |
| 0x02 | z32 | `[uint32 header: (count-1)<<16 \| offset*4+4] [count × uint32 data]...` terminated by `0x00000000` |
| 0x03 | invert | No data. XOR all screen bytes with 0xFF. |
| 0x04 | lines | `[uint16 byte_count] [uint16 byte_offset] [byte_count bytes of data]` |

**Note:** z16 and z32 operate on **vertical-packed** data (columns contiguous in memory). The decoder unpacks/repacks accordingly, matching `bitmap::raw_values<T>()`.

### 8.3. Add round-trip unit tests — DONE

**Implementation:** [src/test/test_decoder.cpp](../src/test/test_decoder.cpp) — 5 basic `apply_delta` tests + 16 round-trip tests covering all codecs individually and combined, including progressive multi-frame convergence. All tests verify bit-for-bit equality between encoder's result bitmap and decoder's reconstructed bitmap.

### 8.4. Make byterate a per-frame input — DONE (by design)

The extracted `encode_frame()` takes `budget` directly. The existing `compressor_helper` computes `budget = byterate_ * ticks` internally, which is fine for the batch path. The streaming path passes a dynamic budget from `adaptive_rate_controller`. No code change needed.

---

## 9. Linux Test Player

A standalone binary for testing the streaming protocol without a real Mac.

### Features

- Opens a UDP socket, sends HELLO with a profile
- Receives frame packets, calls `apply_delta()` to maintain a local `bitmap`
- Renders the bitmap to an SDL2 window (512×342 scaled up)
- Tracks sequence numbers, detects gaps (missed = received seq > expected seq)
- Sends feedback packets every frame
- Command-line flag for simulated packet loss (drop N% of incoming frames randomly) for testing adaptive rate control
- Optional: write received frames to a `.flim` file for offline comparison

### Build

Separate Makefile target: `make streaming-player`. Links against SDL2 + project common code (bitmap, decoder, profile).

---

## 10. Architecture Decisions Log

| Decision | Rationale |
|---|---|
| One frame = one UDP packet | Simplicity. At group=false with Ethernet-limited byterate, packets fit in standard MTU. Progressive reveal on scene changes is acceptable. |
| Client sends profile, not identity | The client requests a delivery format ("deliver like an se30") rather than identifying as hardware. The profile describes screen size, dithering, filters, codecs, byterate. Profile names are shortcuts. |
| Pixel-perfect server-side simulation | More complex than conservative estimation, but far more bandwidth-efficient. Every encoded byte is useful because the server knows exactly what the client has. |
| AIMD rate control | Simple, well-understood, TCP-inspired. Can be replaced with something smarter later. |
| No sound initially | Focus on getting video right. If video streaming works, sound is added into the frame packet (same format as batch `.flim` frames). If video doesn't work, sound won't save it. |
| Decoder as a first-class primitive | Needed for server simulation AND test player AND potentially `flimutil` inspection. Pays for itself many times over. Already partially exists in the codebase. |
| `transport` abstraction | UDP today, but serial/LocalTalk possible later for non-Ethernet Macs. |
| Extract `encode_frame()` now | Small, safe refactoring with zero behavior change. Makes the core encoding reusable for streaming without touching the batch pipeline. |

---

## 11. Open Questions

- **Initial screen sync.** How does the client get the first frame? Options: (a) start from blank screen, let delta encoding converge naturally (may take a second or two of visual garbage), (b) server sends a full uncompressed bitmap as frame 0, (c) use the "initial frame" mechanism from the batch `.flim` format. Option (a) is simplest and works fine since the encoder will prioritize the biggest deltas first.

- **Seeking / pause in file playback.** If streaming from a file, can the client request "jump to time T"? This would require the server to re-initialize the ditherer and screen state. Not needed for v1 but the protocol could be extended with control messages.

- **Multiple clients.** Could multiple Macs watch the same stream? Each would need independent `client_state_tracker` and `adaptive_rate_controller` (since they may have different loss patterns), but could share the ditherer output. Not a priority but the object model supports it — `streaming_session` could manage multiple `{tracker, rate_ctrl, transport}` tuples.

- **Ethernet byterate discovery.** The actual sustainable throughput depends on the Mac's network stack + CPU. Rather than guessing, the AIMD rate controller will find the right level automatically. Starting at 50% of profile byterate is a safe default.

- **Z16 vs Z32 for streaming.** Z32 produces better quality per byte for high-bandwidth scenarios. Z16 may be better for low-bandwidth streaming where the smaller header overhead matters. The codec selection is per-frame and automatic (try all, pick best), so this should work out naturally.

---

## 12. File Map (new + modified)

### New files (streaming implementation)
- `src/streaming/client_state_tracker.hpp` / `.cpp` — pixel-perfect screen simulation ✓
- `src/test/test_client_state_tracker.cpp` — unit tests for `client_state_tracker` ✓
- `src/streaming_session.hpp` / `.cpp` — top-level streaming session
- `src/streaming_encoder.hpp` / `.cpp` — wraps `encode_frame()` for streaming
- `src/adaptive_rate_controller.hpp` / `.cpp` — AIMD rate control
- `src/udp_transport.hpp` / `.cpp` — UDP transport implementation
- `src/transport.hpp` — abstract transport interface
- `src/streaming_player.cpp` — Linux test player (separate binary)
- `src/protocol.hpp` — packet format definitions and serialization

### New files (Phase 1 refactoring)
- `src/encode_frame.hpp` / `.cpp` — extracted single-frame encoding function ✓
- `src/test/test_encode_frame.cpp` — unit tests for `encode_frame()` ✓
- `src/decoder.hpp` / `.cpp` — frame delta decoder (`apply_delta()`) ✓
- `src/test/test_decoder.cpp` — round-trip encode+decode tests ✓

### Modified files (Phase 1 refactoring)
- `src/compressor_helper.cpp` — refactored `add()` to call `encode_frame()` internally ✓
- `src/Makefile` — added new source files and test targets ✓
- `src/compressor.hpp` — inline `#### Decompresses` block replaced with `apply_delta()` call ✓
