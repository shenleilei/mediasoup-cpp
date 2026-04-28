# Bugfix Analysis

## Summary

After the first runtime follow-up pass, the remaining `claude_review.md` findings still cluster into five confirmed defect areas:

- sender pacing still allows retransmission backlog to indefinitely block fresh video
- `RoomRegistry` heartbeat and dead-node eviction still hold command-path assumptions that create contention and pipeline failure ambiguity
- shared FFmpeg wrappers still expose inconsistent null-guard behavior
- lifecycle/event infrastructure still has avoidable ambiguity and cleanup fragility (`@close` semantics, listener ownership, request-id wrap-around, copyable endpoint objects)
- several operational guardrails remain missing or undocumented (subscriber jitter fallback, recorder pending-audio drops, RTCP SR monotonicity, QoS override housekeeping, thread-boundary docs, explicit worker-crash behavior)

## Reproduction

1. Inspect `client/SenderTransportController.h` and existing bitrate-allocation tests; retransmission is still drained ahead of fresh video with no fairness bound.
2. Inspect `src/RoomRegistry.cpp` and `src/RoomRegistrySync.cpp`; `heartbeat()` still holds the command mutex across node refresh + eviction and `evictDeadNodes()` still ignores pipeline append failures.
3. Inspect `common/ffmpeg/BitstreamFilter.cpp`, `common/ffmpeg/Decoder.cpp`, and `common/ffmpeg/Encoder.cpp`; null pointer arguments still reach raw FFmpeg calls.
4. Inspect `src/EventEmitter.h`, `src/Transport.cpp`, `src/Router.cpp`, `src/Consumer.cpp`, `src/Producer.h`, `src/Consumer.h`, and `src/Channel.cpp`; listener removal still lacks direct ownership handles in some paths, close semantics are not reasoned explicitly, and `Channel` request IDs still wrap without collision avoidance.
5. Inspect `src/RoomRegistryPubSub.cpp`, `src/Recorder.cpp`, `client/RtcpHandler.h`, `client/qos/QosController.h`, and `src/SignalingServer.h`; fallback/observability/thread-boundary behavior remains incomplete or undocumented.

## Observed Behavior

- continuous retransmission backlog can starve fresh video indefinitely
- Redis command mutex hold time remains broader than necessary for heartbeat maintenance
- FFmpeg wrapper misuse can still crash instead of failing fast
- lifecycle cleanup remains more implicit than explicit, especially around internal close semantics and listener ownership
- several fallback and diagnostic paths remain opaque in production

## Expected Behavior

- retransmission priority SHALL remain higher than fresh video without allowing indefinite starvation
- registry heartbeat SHALL minimize command mutex hold time and detect pipeline append failure explicitly
- shared FFmpeg wrappers SHALL reject null output/input arguments consistently
- internal close semantics and listener ownership SHALL be explicit enough to avoid cleanup drift
- operational fallbacks and long-lived state cleanup SHALL be observable and documented

## Known Scope

- `client/SenderTransportController.h`
- `client/RtcpHandler.h`
- `client/qos/QosController.h`
- `common/ffmpeg/BitstreamFilter.cpp`
- `common/ffmpeg/Decoder.cpp`
- `common/ffmpeg/Encoder.cpp`
- `src/EventEmitter.h`
- `src/Transport.cpp`
- `src/Transport.h`
- `src/Router.cpp`
- `src/Producer.cpp`
- `src/Producer.h`
- `src/Consumer.cpp`
- `src/Consumer.h`
- `src/Channel.cpp`
- `src/Channel.h`
- `src/RoomRegistry.cpp`
- `src/RoomRegistrySync.cpp`
- `src/RoomRegistryPubSub.cpp`
- `src/Recorder.cpp`
- `src/Recorder.h`
- `src/SignalingServer.h`
- affected tests under `tests/`
- accepted specs under `specs/current/`

## Must Not Regress

- audio-before-video pacing priority
- current worker crash handling contract that notifies clients with `serverRestart` and allows new rooms after respawn
- existing room-registry redirect and sync correctness outside the narrowed maintenance paths
- current Producer/Consumer/Transport public event names
- existing Recorder pending-audio capacity cap and RTCP packet generation semantics

## Out Of Scope

- a full Room/Peer/Transport/Producer/Consumer state-machine redesign
- replacing the dual threaded / non-threaded `Channel` model with a single architecture
- lossless room recovery across worker crashes

## Acceptance Criteria

### Requirement 1

Normal pacing SHALL preserve retransmission priority without indefinite fresh-video starvation.

#### Scenario: Continuous retransmission backlog

- WHEN fresh video and retransmission packets remain queued across many pacing ticks
- THEN retransmission still receives higher-priority service
- AND fresh video still makes bounded progress over time

### Requirement 2

Registry heartbeat maintenance SHALL narrow mutex hold time and reject pipeline append failure explicitly.

#### Scenario: Dead-node eviction

- WHEN heartbeat runs node refresh and dead-node eviction
- THEN unrelated registry operations are not blocked across both phases under one outer mutex
- AND append failure aborts eviction before mismatched reply consumption

### Requirement 3

Shared FFmpeg wrappers SHALL reject null API misuse consistently.

#### Scenario: Null wrapper arguments

- WHEN callers pass null codec parameters, frames, or packets into shared wrapper entrypoints
- THEN the wrappers throw explicit errors before reaching raw FFmpeg calls

### Requirement 4

Lifecycle cleanup SHALL use explicit internal close semantics and direct listener ownership.

#### Scenario: terminal close paths

- WHEN producers, consumers, or transports close for different reasons
- THEN internal close listeners receive an explicit terminal close signal
- AND channel listeners are unregistered by owned listener ID rather than broad event-name removal
- AND request ID wrap-around cannot overwrite a still-pending request

### Requirement 5

Operational fallback behavior SHALL remain observable and documented.

#### Scenario: fallback / long-pause runtime conditions

- WHEN subscriber jitter entropy falls back, recorder drops pending audio, RTCP SR clocking advances, or paused QoS controllers retain expired overrides
- THEN the system provides deterministic cleanup or explicit diagnostics
- AND accepted behavior is documented for worker-crash teardown and thread-boundary assumptions

## Regression Expectations

- targeted sender-transport bitrate-allocation tests
- targeted FFmpeg wrapper misuse tests
- targeted EventEmitter / lifecycle cleanup tests
- targeted room-registry maintenance tests
- targeted QoS / RTCP / Recorder unit tests where behavior changes
- main binary build plus focused unit/review-fix suites
