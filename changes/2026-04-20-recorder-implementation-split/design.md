# Design

## Goal

Turn `PeerRecorder` into a normal C++ component with:

- a declaration-oriented header
- a `.cpp` implementation file

without changing recorder semantics.

## Approach

### 1. Keep the public interface stable

`Recorder.h` should continue to expose:

- `VideoCodec`
- `PeerRecorder`
- current public methods such as `createSocket()`, `start()`, `stop()`, `appendQosSnapshot()`

The header will retain private method and member declarations, but the logic bodies move to `Recorder.cpp`.

### 2. Preserve recorder-local policy

This split is structural only.

The following remain recorder-owned and stay inside `PeerRecorder` implementation:

- RTP receive loop
- VP8 descriptor stripping
- H264 depacketization
- deferred H264 header policy
- mux write policy
- QoS sidecar timing/writing

### 3. Keep current test access working

Current tests use `PeerRecorderTestAccess` to reach a few private helpers/fields.

The split should preserve the friend declaration and whatever private declarations are required so those tests continue to work after linking against `Recorder.cpp`.

### 4. Fix build-link assumptions explicitly

Because `PeerRecorder` will stop being header-only, targets that currently compile because they include `Recorder.h` but do not link a translation unit with its implementation must be updated.

The expected primary solution is:

- add `src/Recorder.cpp` to `mediasoup_lib`
- update any test targets using `PeerRecorder` but not linking `mediasoup_lib`

## Risks

- missing one or more targets that relied on header-only linkage
- subtle test-access breakage if private declarations move incorrectly
- accidental behavior drift while moving large inline methods

## Verification Strategy

- build targets touching recorder production and tests
- targeted recorder-focused unit/integration tests
- existing shared-media/shared-ffmpeg regressions should remain green
