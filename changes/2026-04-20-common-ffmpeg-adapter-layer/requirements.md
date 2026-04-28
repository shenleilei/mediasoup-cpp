# Problem Statement

The repository currently performs most FFmpeg work directly inside role-specific runtime code:

- `client/main.cpp` opens inputs, probes streams, allocates packets/frames, creates decoders/encoders, drives bitstream filters, and manually frees resources.
- `src/Recorder.h` allocates and manages output muxing state, streams, AVIO handles, and packet writes directly.

This creates three concrete problems:

1. FFmpeg lifecycle management is highly manual and C-style, with repeated allocate/open/free paths.
2. Plain-client and recorder both perform overlapping classes of FFmpeg work:
   - demux / mux
   - decode / encode
   - read / write
   - bitstream filter setup
3. Error handling and cleanup semantics are inconsistent and hard to review across flows.

The project already has a shared media protocol helper layer. It now needs a shared FFmpeg adapter layer so recorder and plain-client can both depend on the same resource-management and operation wrappers rather than embedding raw `av_*` control flow throughout their runtime files.

## Business Goal

Create a repository-level `common/ffmpeg` adapter layer that centralizes FFmpeg resource ownership and core operations for both recorder and plain-client, while preserving their separate runtime pipelines and media semantics.

## In Scope

- introduce `common/ffmpeg/`
- add RAII ownership wrappers for the FFmpeg resource types currently managed manually
- add shared wrappers for the core FFmpeg operation families used by recorder and plain-client:
  - input open / stream discovery / packet read
  - output alloc / stream creation / IO open / header / trailer / packet write
  - decoder open / send packet / receive frame
  - encoder open / send frame / receive packet
  - bitstream filter alloc / init / send packet / receive packet
  - common error and timebase helpers
- migrate plain-client and recorder call sites to the shared FFmpeg adapter layer
- keep root build and standalone `client/` build working

## Out Of Scope

- merging recorder and plain-client runtime pipelines
- creating a generic media framework above FFmpeg
- changing RTP/H264 protocol helper behavior as a primary goal
- redesigning recorder threading or plain-client threading
- rewriting QoS or signaling logic

## Design Constraints

- Default ownership SHALL use `std::unique_ptr` with custom deleters or wrapper classes with unique ownership semantics.
- `std::shared_ptr` SHALL only be introduced when shared ownership is demonstrably required by the runtime design.
- `common/ffmpeg` SHALL centralize FFmpeg operations and resource cleanup, but SHALL NOT absorb recorder/plain-client business flow or runtime ownership.

## Scenarios

### Scenario 1: Plain-client bootstrap

A developer opens input media, configures decoders/encoders, or drives a bitstream filter through shared FFmpeg adapters instead of direct raw `av_*` lifecycle code in `client/main.cpp`.

### Scenario 2: Recorder mux lifecycle

A developer changes output format allocation, stream creation, header/trailer emission, or packet writing through shared FFmpeg adapters instead of raw mux ownership code in `src/Recorder.h`.

### Scenario 3: Cleanup on failure

A partial initialization failure in recorder or plain-client does not require hand-written mirrored free paths for every allocated FFmpeg object.

## Acceptance Criteria

1. The repository SHALL expose a shared `common/ffmpeg` layer outside both recorder-private and plain-client runtime files.
2. The shared layer SHALL centralize ownership and cleanup for the FFmpeg resource types currently allocated manually in recorder/plain-client.
3. Plain-client SHALL no longer directly own raw FFmpeg allocation/free/open/close control flow for its main input/codec/bsf resources.
4. Recorder SHALL no longer directly own raw FFmpeg output allocation/open/close control flow for its mux context and output IO lifecycle.
5. Root build and standalone `client/` build SHALL both compile after introducing the shared FFmpeg layer.
6. Existing recorder/plain-client affected tests SHALL continue to pass after the migration.

## Non-Functional Requirements

- Preserve current runtime behavior unless an accepted spec explicitly changes it.
- Keep adapter APIs explicit and reviewable; do not hide control flow behind clever abstractions.
- Prefer small wrapper classes around real FFmpeg concepts instead of one oversized manager object.
- Keep migration incremental and verifiable by operation family.

## Edge Cases And Failure Cases

- input open failure
- stream-info discovery failure
- missing decoder/encoder/bitstream filter
- encoder recreation during plain-client QoS-driven reconfigure
- recorder deferred header failure and trailer cleanup
- packet/frame allocation failure
- partial initialization after some FFmpeg resources already exist

## Open Questions

- whether output mux wrapper and codec wrappers should live in separate files or one narrow module per concept
- whether threaded plain-client audio/video workers should be migrated in the same wave or immediately after the main/legacy path
