# Problem Statement

The repository currently mixes protocol-level media byte manipulation into role-specific runtime files:

- plain-client send-side H264 RTP packetization appears in both `client/main.cpp` and `client/NetworkThread.h`
- recorder-side RTP parsing and Annex-B/AVCC conversion live inside `src/Recorder.h`
- some helper tests currently depend on recorder-private test access rather than a stable public helper API

This creates two concrete problems:

1. low-level RTP/H264 fixes must be repeated or re-reviewed in multiple runtime owners
2. byte-level media helpers are hidden behind recorder/plain-client runtime code instead of being available as a neutral shared library boundary

At the same time, recorder and plain-client are not one runtime subsystem:

- plain-client is a sender-side publish path
- recorder is a receiver-side local recording path

They may be mutually exclusive in deployment role, but they do not share:

- socket ownership
- thread model
- RTCP policy
- FFmpeg context ownership
- QoS control logic

The project therefore needs a shared media *protocol helper* layer, not a shared media *runtime*.

## Business Goal

Create a repository-level shared media protocol helper layer for reusable byte-level RTP/H264 utilities, while preserving explicit and separate runtime ownership for recorder and plain-client.

## In Scope

- a new shared repo-level media helper directory
- phase-1 extraction of duplicated or mislocated pure protocol helpers
- migration of existing recorder/plain-client call sites to phase-1 helpers
- public tests for the shared helper API
- build wiring that works from both the root build and standalone `client/` build

## Out Of Scope

- merging recorder and plain-client runtimes
- introducing a generic shared media framework
- moving RTCP/NACK/PLI policy into the shared layer
- moving FFmpeg mux/decode/encode ownership into the shared layer
- redesigning recorder threading or plain-client threading
- changing room/worker/recording product behavior as a primary goal

## Phase-1 Scope

Phase 1 SHALL be limited to helpers that meet all of these rules:

- protocol-level and reusable across roles
- no socket, queue, thread, room, peer, logger, or QoS ownership
- no dependency on FFmpeg types
- callable through a public API without recorder/plain-client private test access

Phase-1 candidate helpers:

- RTP header parsing
- H264 Annex-B RTP packet emission / fragmentation
- Annex-B to AVCC conversion

Phase 1 SHALL NOT automatically include H264 depacketization unless the API can be expressed as caller-owned assembly state with no recorder-specific policy.

## Scenarios

### Scenario 1: One packetizer implementation

A developer fixes H264 RTP fragmentation or marker-bit behavior in one shared helper and both plain-client send paths consume the same implementation.

### Scenario 2: Public helper tests

A developer can test RTP/H264 helper behavior directly through a shared API instead of invoking recorder-private accessors.

### Scenario 3: Runtime ownership remains obvious

A reviewer can still determine which code owns sockets, threads, FFmpeg contexts, and RTCP/QoS policy for:

- recorder
- plain-client legacy path
- plain-client threaded path

without tracing through a generic shared media runtime.

## Acceptance Criteria

1. The repository SHALL expose a shared media helper layer outside both `src/` recorder-private code and `client/` plain-client runtime code.
2. The shared layer SHALL only contain protocol/helper code and SHALL NOT own sockets, threads, queues, RTCP policy, QoS logic, room state, or FFmpeg contexts.
3. Phase 1 SHALL eliminate duplicated H264 RTP packetizer logic across the plain-client legacy and threaded send paths.
4. Phase 1 SHALL expose RTP header parsing and Annex-B/AVCC conversion through a public shared API.
5. Shared helper tests SHALL target the shared public API directly and SHALL no longer depend on recorder-private access for the migrated helpers.
6. Root build and standalone `client/` build SHALL both compile after the shared layer is introduced.
7. Recorder and plain-client SHALL retain separate lifecycle/threading/FFmpeg orchestration code after the extraction.

## Non-Functional Requirements

- Preserve current external behavior unless an accepted spec explicitly changes it.
- Prefer deterministic, side-effect-free helpers with explicit inputs and outputs.
- Keep hot-path APIs allocation-aware where practical; do not force hidden heap work when a caller-provided sink or scratch buffer is sufficient.
- Keep migration incremental so each extracted helper can be validated independently.

## Edge Cases And Failure Cases

- malformed or truncated RTP headers
- H264 single-NAL packetization
- H264 FU-A fragmentation and end-marker handling
- Annex-B inputs with multiple NAL units
- AVCC conversion correctness after reconstructed Annex-B frame assembly
- accidental leakage of recorder-specific flush/SPS/PPS/timestamp policy into the shared layer

## Open Questions

- whether the shared helper directory should be named `common/media/`, `shared/media/`, or another repo-level equivalent
- whether H264 depacketization should enter phase 2 only after `Recorder.h` is split, or earlier via a strict caller-owned assembler API

## Current Phase-1 Completion Notes

The current implementation wave completes phase 1 for:

- RTP header parsing
- H264 RTP packetization
- Annex-B to AVCC conversion

The current implementation wave intentionally does not include:

- H264 depacketization
- shared RTCP helpers
- shared FFmpeg ownership helpers
- shared recorder/plain-client runtime orchestration
