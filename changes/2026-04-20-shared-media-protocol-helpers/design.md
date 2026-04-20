# Design

## Design Standard

This change is a library-boundary cleanup, not a runtime rewrite.

The highest-standard version of this design follows four rules:

1. shared code must be protocol-level only
2. shared APIs must be explicit about ownership and side effects
3. phase 1 must only extract helpers with proven reuse or immediate duplication payoff
4. build and test wiring must be treated as first-class design requirements, not implementation afterthoughts

## Current Reality

### Truly duplicated or misplaced helpers

- H264 RTP packetizer duplicated in:
  - `client/main.cpp`
  - `client/NetworkThread.h`
- RTP header parser embedded in:
  - `src/Recorder.h`
- Annex-B to AVCC conversion embedded in:
  - `src/Recorder.h`

### Role-specific code that must stay local

- plain-client:
  - send loop
  - pacing
  - RTCP/NACK/PLI behavior
  - QoS control
- recorder:
  - UDP receive loop
  - frame timestamping / flush policy
  - muxing
  - QoS sidecar output

### Important nuance about depacketization

Recorder-side H264 depacketization is not currently a pure helper. It is entangled with:

- SPS/PPS capture
- keyframe detection
- access-unit buffer ownership
- timestamp baseline and flush policy

Therefore H264 depacketization can only become shared if the shared API is reduced to byte assembly primitives plus caller-owned state, without inheriting recorder policy.

## Proposed Repository Structure

Introduce a new repo-level shared directory:

```text
common/media/
  rtp/
    RtpHeader.h
    H264Packetizer.h
    H264Packetizer.cpp
  h264/
    AnnexB.h
    AnnexB.cpp
    Avcc.h
    Avcc.cpp
```

Phase 2 may add:

```text
common/media/h264/
  H264AuAssembler.h
  H264AuAssembler.cpp
```

but only if the API remains caller-owned and policy-free.

## API Boundary Rules

### Shared helper allowed responsibilities

- parse RTP header fields from raw bytes
- iterate or emit H264 RTP payload fragments from Annex-B input
- convert Annex-B access units to AVCC framing
- expose minimal state structs required for pure byte-level assembly

### Shared helper forbidden responsibilities

- calling `send()`
- queueing packets
- pacing
- updating RTCP state
- owning recorder buffers or timestamps
- owning FFmpeg objects
- logging policy
- room/peer/session/QoS semantics

## API Shape Requirements

### RtpHeader

`RtpHeader` should be a parse/view helper only.

Requirements:

- no dependency on recorder
- parse failure is explicit
- header-extension and CSRC handling stay inside the parser

### H264Packetizer

The packetizer MUST be a pure transform plus sink callback or explicit packet sink object.

It MUST NOT:

- own a file descriptor
- call `send()`
- write into `RtcpContext`
- know about pacing queues

Recommended shape:

- input: Annex-B access unit bytes, payload type, RTP timestamp, SSRC, mutable sequence number
- output: packets emitted through a caller-provided sink callback

That allows:

- legacy plain-client to sink packets into `send() + onVideoRtpSent`
- threaded plain-client to sink packets into `pacingEnqueue`

without duplicating fragmentation logic.

### Annex-B / AVCC conversion

Conversion helpers should remain pure byte transforms:

- input: Annex-B NAL unit buffer
- output: AVCC-framed buffer

No recorder-specific mux or SPS/PPS policy should live here.

### H264 depacketization / assembly

If phase 2 proceeds, the shared abstraction must be a caller-owned assembler, not a recorder object.

It may own only protocol-level assembly state such as:

- in-progress access-unit byte buffer
- FU-A partial reconstruction state

It MUST NOT own:

- recorder timestamps
- keyframe publication policy
- SPS/PPS persistence beyond what is required for protocol-level reconstruction
- flush-to-mux actions

If this clean separation is not possible, depacketization stays role-local until `Recorder.h` is split further.

## Migration Plan

### Phase 1: Extract proven pure helpers

Extract:

- `RtpHeader`
- `H264Packetizer`
- Annex-B to AVCC conversion

Do not extract depacketization yet.

### Phase 2: Migrate plain-client send paths

Replace duplicated H264 send logic in:

- `client/main.cpp`
- `client/NetworkThread.h`

with the shared packetizer and role-local sinks.

### Phase 3: Migrate public tests

Move helper tests off recorder-private access so that:

- RTP header parsing
- Annex-B/AVCC conversion
- H264 packetizer behavior

are tested through public shared APIs.

### Phase 4: Re-evaluate H264 depacketization

Only after phase 1 is stable and recorder boundaries are clearer:

- design a caller-owned `H264AuAssembler`, or
- explicitly defer shared depacketization as not yet cleanly extractable

### Phase 5: Optional follow-up helpers

Only after the above:

- evaluate `Opus` packetization
- evaluate additional RTP utility helpers

## Build Strategy

The shared layer must compile in both build modes:

- root repo build
- standalone `client/` build

The design therefore requires an explicit build solution, not an implicit include-path accident.

Acceptable implementations include:

- a small shared target reachable from both builds
- or direct source reuse with a single repo-level include path contract

Whichever route is chosen, both build entrypoints are mandatory verification gates.

## Test Strategy

### Shared helper tests

Add direct tests for public shared APIs:

- RTP header parse success/failure
- H264 single-NAL packetization
- H264 FU-A fragmentation boundaries
- marker-bit propagation
- Annex-B to AVCC conversion

### Regression checks

After migration:

- plain-client build
- affected plain-client runtime tests
- recorder-related tests

### Test migration requirement

For any helper moved into the shared layer, tests must stop depending on recorder-private access for that helper.

## Alternatives Considered

### One shared media runtime for recorder and plain-client

Rejected because sender and recorder runtime ownership are fundamentally different and should remain obvious in code review.

### Extract packetizer and depacketizer together in phase 1

Rejected at the highest standard because packetizer duplication is proven today, while depacketizer still carries recorder-local policy baggage.

### Keep helpers under `src/`

Rejected because that would misrepresent cross-cutting code as server-only infrastructure.

## Risks And Mitigations

- Risk: packetizer helper drifts into transport/runtime behavior.
  Mitigation: enforce callback/sink output with no direct I/O in the shared API.

- Risk: shared depacketizer accidentally embeds recorder policy.
  Mitigation: keep depacketizer out of phase 1 and require caller-owned state if it is later extracted.

- Risk: build wiring works in one entrypoint but breaks in the other.
  Mitigation: make both root and standalone client builds explicit delivery gates.

- Risk: tests keep passing through legacy private access while shared APIs are untested.
  Mitigation: require direct public-API tests for every migrated helper.

## Rollout Notes

- Phase 1 starts with helpers that are already duplicated or clearly protocol-local.
- `Opus` and broader RTCP helpers are not first-wave goals.
- Recorder runtime splitting remains a separate follow-up change.

## Follow-up Gates

`H264Depacketizer` is not an automatic phase-2 task.

It should be reconsidered only when at least one of these becomes true:

1. a second receiver-side consumer of reconstructed H264 access units exists
   - for example CDN push, RTP ingest/relay, or server-side remux/transcode output
2. `Recorder.h` has been split enough that protocol assembly state can be separated cleanly from recorder-local timestamp/flush/mux policy
3. receiver-side H264 reconstruction logic becomes duplicated in more than one runtime owner

Until one of those gates is met, receiver-side H264 reassembly should remain local to recorder.
