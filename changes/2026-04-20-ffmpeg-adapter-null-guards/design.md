# Design

## Context

The repository now has a shared FFmpeg adapter layer used by both recorder and plain-client code.

That layer should be safer than raw FFmpeg calls, especially because its classes are intentionally movable and often stored in `std::optional`.

## Root Cause

Each adapter class can enter an empty state through:

- default construction
- move-from after transfer
- cleanup/reset paths

Some methods already recognize that state and fail safely, while others still assume a live FFmpeg context and dereference null.

The missing piece is a single, consistent rule for how empty-state calls should behave.

## Fix Strategy

### 1. Keep the existing distinction between query and operation methods

Do not change the meaning of methods that already behave like safe queries:

- `InputFormat::FindFirstStreamIndex()` returns `-1` on empty
- `InputFormat::StreamAt()` returns `nullptr` on empty
- cleanup/no-op methods such as `Close()` and `WriteTrailer()` remain safe on empty where that behavior already exists

### 2. Guard all operational methods with explicit runtime checks

For methods that require a live FFmpeg context to do useful work:

- add a small local helper per adapter source file to validate the internal pointer/context
- throw `std::runtime_error` with a clear adapter-specific message when empty

This applies to:

- `InputFormat::FindStreamInfo()`
- `InputFormat::ReadPacket()`
- `OutputFormat::NewStream()`
- `OutputFormat::OpenIo()`
- `OutputFormat::WriteHeader()`
- `OutputFormat::WriteInterleavedFrame()`
- `Decoder::SendPacket()`
- `Decoder::ReceiveFrame()`
- `Encoder::SendFrame()`
- `Encoder::ReceivePacket()`
- `BitstreamFilter::SendPacket()`
- `BitstreamFilter::ReceivePacket()`

### 3. Add focused unit tests for default and moved-from states

Extend `tests/test_common_ffmpeg.cpp` to cover:

- default-constructed adapters
- moved-from adapters after ownership transfer
- preserved sentinel behavior for `InputFormat` queries

The tests should validate exception behavior directly and avoid requiring a crash-based repro.

## Risk Assessment

- changing empty-state behavior from “segfault” to exception can surface latent misuse in tests or call sites
- the rule must stay narrow enough that cleanup and query methods keep their existing benign semantics
- exception messages should be stable enough to make failures diagnosable without overfitting tests to long strings

## Test Strategy

- extend `tests/test_common_ffmpeg.cpp`
- build and run `mediasoup_tests`

No broader integration suite is required because this change is local to the adapter layer and its unit coverage.

## Observability

- adapter misuse will now surface as controlled exceptions instead of crashes
- no new logs or metrics are required

## Rollout Notes

- no API contract or accepted behavior doc changes are expected beyond safer misuse handling
- if any call site now trips a new exception, that will expose an existing misuse that should be fixed in a follow-up rather than hidden again
