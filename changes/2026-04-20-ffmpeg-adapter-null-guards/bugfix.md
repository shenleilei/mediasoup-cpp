# FFmpeg Adapter Null Guards

## Summary

The shared `common/ffmpeg` adapter layer still handles empty or moved-from objects inconsistently.

Today:

- some methods safely no-op or return sentinel values when the adapter has no FFmpeg context
- some methods immediately dereference a null internal pointer and can segfault

This makes default-constructed, reset, or moved-from adapter objects unsafe in unpredictable ways.

## Reproduction

1. Review `common/ffmpeg/InputFormat.cpp`, `OutputFormat.cpp`, `Decoder.cpp`, `Encoder.cpp`, and `BitstreamFilter.cpp`.
2. Observe that a few methods already guard null state, such as `InputFormat::FindFirstStreamIndex()`, `InputFormat::StreamAt()`, `OutputFormat::Close()`, and `OutputFormat::WriteTrailer()`.
3. Observe that adjacent methods like `InputFormat::FindStreamInfo()`, `InputFormat::ReadPacket()`, `OutputFormat::NewStream()`, `OutputFormat::OpenIo()`, `OutputFormat::WriteHeader()`, `OutputFormat::WriteInterleavedFrame()`, and codec/filter send/receive methods still call FFmpeg with unchecked null internals.
4. Construct a default or moved-from adapter object and call one of those unchecked methods.

## Observed Behavior

- some empty-state adapter calls fail safely
- some empty-state adapter calls segfault by passing null FFmpeg contexts into `av_*` functions

## Expected Behavior

- shared FFmpeg adapters SHALL treat empty-state misuse consistently
- query-style methods that already expose safe sentinel behavior MAY keep that behavior
- operational methods that require a live FFmpeg context MUST reject empty-state use explicitly instead of crashing

## Known Scope

- `common/ffmpeg/InputFormat.*`
- `common/ffmpeg/OutputFormat.*`
- `common/ffmpeg/Decoder.*`
- `common/ffmpeg/Encoder.*`
- `common/ffmpeg/BitstreamFilter.*`
- `tests/test_common_ffmpeg.cpp`

## Must Not Regress

- existing FFmpeg adapter success-path behavior
- explicit trailer writes and best-effort close behavior already fixed in `OutputFormat`
- current query-style sentinel behavior for `InputFormat::FindFirstStreamIndex()` and `InputFormat::StreamAt()`
- recorder and plain-client call sites that already use initialized adapters correctly

## Suspected Root Cause

Confidence: high.

The adapter layer supports:

- default construction
- move construction / move assignment
- optional/reset-style ownership in recorder and plain-client code

That means “object exists, internal FFmpeg context is null” is a real runtime state.

The original implementation guarded some cleanup/query methods, but not all operational methods, so null-state handling drifted method by method.

## Acceptance Criteria

### Requirement 1

Shared FFmpeg adapter operational methods SHALL reject empty-state use explicitly.

#### Scenario: empty input format

- WHEN `InputFormat::FindStreamInfo()` or `InputFormat::ReadPacket()` is called on an empty or moved-from `InputFormat`
- THEN the method SHALL throw a clear runtime error instead of crashing

#### Scenario: empty output format

- WHEN `OutputFormat::NewStream()`, `OpenIo()`, `WriteHeader()`, or `WriteInterleavedFrame()` is called on an empty or moved-from `OutputFormat`
- THEN the method SHALL throw a clear runtime error instead of crashing

#### Scenario: empty codec or bitstream-filter adapter

- WHEN `Decoder`, `Encoder`, or `BitstreamFilter` send/receive methods are called on an empty or moved-from adapter
- THEN the method SHALL throw a clear runtime error instead of crashing

### Requirement 2

Existing safe sentinel behavior for query-like methods SHALL remain stable where already established.

#### Scenario: empty input format queries

- WHEN `FindFirstStreamIndex()` is called on an empty `InputFormat`
- THEN it SHALL keep returning `-1`

- WHEN `StreamAt()` is called on an empty `InputFormat`
- THEN it SHALL keep returning `nullptr`

### Requirement 3

The adapter layer SHALL gain direct regression coverage for empty-state use.

#### Scenario: default or moved-from adapters

- WHEN tests exercise operational methods on default-constructed or moved-from adapters
- THEN they SHALL observe controlled exceptions instead of process crashes

## Regression Expectations

- existing FFmpeg adapter unit tests continue to pass
- new regression coverage is added for empty-state `InputFormat`, `OutputFormat`, `Decoder`, `Encoder`, and `BitstreamFilter` usage
- manual smoke checks are not required if the adapter unit coverage remains sufficient
