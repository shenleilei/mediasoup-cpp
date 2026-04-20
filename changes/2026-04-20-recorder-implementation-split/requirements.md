# Problem Statement

`src/Recorder.h` still contains the full `PeerRecorder` implementation inline in a header.

That creates several problems:

- rebuild coupling is broader than necessary for recorder changes
- recorder logic is harder to review because declaration, runtime policy, RTP parsing, muxing, and QoS sidecar writing are all interleaved in one header
- now that `common/media` and `common/ffmpeg` exist, recorder is one of the remaining large runtime hotspots that still has not been turned into a normal declaration/implementation boundary

## Business Goal

Split `PeerRecorder` into a normal header/implementation pair while preserving current recorder behavior and external interfaces.

## In Scope

- `src/Recorder.h`
- new `src/Recorder.cpp`
- build wiring for all targets that use `PeerRecorder`
- recorder-related tests and affected documentation

## Out Of Scope

- introducing `H264Depacketizer`
- redesigning recorder threading or runtime behavior
- changing recorder public APIs unless required for the declaration/implementation split
- broader recording feature work

## Acceptance Criteria

1. `src/Recorder.h` SHALL primarily contain declarations instead of the full runtime implementation.
2. `PeerRecorder` runtime logic SHALL move into `src/Recorder.cpp`.
3. Existing public usage of `PeerRecorder` in production and tests SHALL continue to compile after the split.
4. Recorder behavior SHALL remain unchanged for:
   - socket creation
   - start/stop
   - RTP receive and depacketize
   - deferred H264 header handling
   - QoS sidecar writing
5. All affected targets SHALL link successfully after `PeerRecorder` stops being header-only.

## Non-Functional Requirements

- keep the public class shape stable where practical
- keep the split narrow; do not mix in additional recorder redesign
- preserve current test-only access where still needed
