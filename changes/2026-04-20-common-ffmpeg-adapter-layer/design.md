# Design

## Design Standard

This change is a shared adapter-layer refactor, not a media-pipeline merge.

The highest-standard design follows these rules:

1. shared FFmpeg code centralizes ownership and operations
2. runtime orchestration remains in recorder/plain-client
3. ownership defaults to unique ownership
4. wrappers mirror real FFmpeg concepts and failure points

## Why `unique_ptr` By Default

Most FFmpeg resources used here are single-owner handles:

- `AVFormatContext`
- `AVCodecContext`
- `AVPacket`
- `AVFrame`
- `AVBSFContext`

Using `std::shared_ptr` for all of them would weaken lifetime clarity and make error-path reasoning harder.

Therefore:

- default: `std::unique_ptr` with custom deleter or wrapper class owning a unique handle
- exception: `std::shared_ptr` only when a runtime design truly requires shared ownership

## Proposed Repository Structure

Introduce:

```text
common/ffmpeg/
  AvError.h
  AvError.cpp
  AvPtr.h
  AvTime.h
  InputFormat.h
  InputFormat.cpp
  OutputFormat.h
  OutputFormat.cpp
  Decoder.h
  Decoder.cpp
  Encoder.h
  Encoder.cpp
  BitstreamFilter.h
  BitstreamFilter.cpp
```

Final file grouping may vary, but the conceptual boundaries must hold.

## Shared Layer Responsibilities

### `AvError`

- translate FFmpeg error codes to strings
- provide checked helper functions for common failure paths

### `AvPtr`

- unique-ownership aliases and deleters for:
  - packets
  - frames
  - codec contexts
  - bitstream filters

### `AvTime`

- explicit timebase/rescale helpers used across recorder/plain-client

### `InputFormat`

- `avformat_open_input`
- `avformat_find_stream_info`
- stream access helpers
- packet read helper
- input cleanup

### `OutputFormat`

- `avformat_alloc_output_context2`
- stream creation
- `avio_open`
- header / trailer
- interleaved packet write
- output cleanup

### `Decoder`

- open decoder from codec parameters
- `send_packet`
- `receive_frame`

### `Encoder`

- open encoder with explicit caller configuration
- `send_frame`
- `receive_packet`
- expose underlying context when necessary for existing configuration fields

### `BitstreamFilter`

- allocate/init named filter
- copy input codec parameters and timebase
- `send_packet`
- `receive_packet`

## Shared Layer Non-Responsibilities

- recorder timestamp baseline policy
- recorder deferred-header business policy
- plain-client QoS state machine
- network send/pacing
- RTP/H264 protocol logic already owned by `common/media`
- room/peer/session semantics

## Wrapper Shape

Wrappers should be small and explicit.

Examples:

- `InputFormat::Open(path)`
- `Decoder::OpenFromParameters(codecpar)`
- `Encoder::Create(codecId, configureFn)`
- `BitstreamFilter::Create(name, codecpar, timeBase)`
- `OutputFormat::Create(formatName, outputPath)`

Wrappers may expose raw FFmpeg pointers through `get()` when unavoidable for configuration, but ownership and open/close semantics must remain inside the wrapper.

## Migration Plan

### Phase 1: Foundation

Add:

- `AvError`
- `AvPtr`
- `AvTime`
- `InputFormat`
- `Decoder`
- `Encoder`
- `BitstreamFilter`
- `OutputFormat`

### Phase 2: Plain-client migration

Migrate `client/main.cpp` first:

- input open / stream discovery
- audio decoder/encoder
- video decoder
- H264 bitstream filter
- packet/frame allocation ownership

This phase should remove the main raw `av_*` lifecycle management from the non-threaded path and shared bootstrap.

### Phase 3: Recorder migration

Migrate `src/Recorder.h` to:

- output format allocation/open/close
- stream creation
- header/trailer
- packet write

Recorder policy remains local; only FFmpeg ownership/operations move.

### Phase 4: Threaded plain-client follow-up

If any threaded audio/video worker code still owns raw FFmpeg lifecycles after phase 2, migrate those remaining paths to the shared layer.

## Build Strategy

Like `common/media`, shared FFmpeg sources must be wired into:

- root repo build
- standalone `client/` build

The source list should come from one shared CMake definition to avoid drift.

## Test Strategy

### Compilation gates

- root build:
  - `mediasoup_tests`
  - `mediasoup_qos_unit_tests`
  - `mediasoup-sfu`
- standalone client build:
  - `plain-client`

### Runtime regression gates

- recorder-focused tests already covering start/stop/deferred-header behavior
- shared helper/media tests remain green
- targeted plain-client-related tests or smoke builds

### Adapter-layer tests

Where practical, add focused tests for:

- error formatting
- wrapper cleanup on failure paths

but prefer migration-backed regression coverage over mocking FFmpeg heavily.

## Risks And Mitigations

- Risk: wrappers become too abstract and hide the actual FFmpeg state machine.
  Mitigation: one wrapper per FFmpeg concept, explicit method names, no giant manager.

- Risk: recorder and plain-client still need access to codecpar/context fields.
  Mitigation: allow controlled `get()` access while keeping allocation/open/close inside wrappers.

- Risk: migration becomes too large if every path is touched at once.
  Mitigation: foundation first, then plain-client, then recorder, then leftover threaded paths.

## Rollout Notes

- `common/media` and `common/ffmpeg` are complementary:
  - `common/media`: RTP/H264 byte-level protocol helpers
  - `common/ffmpeg`: demux/mux/encode/decode/bsf adapter layer
- Do not fold them into one module.
