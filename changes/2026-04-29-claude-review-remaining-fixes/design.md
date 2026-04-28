# Design

## Context

The remaining review items are broader than a single hotfix but still share a theme: turn implicit behavior into explicit, verified behavior without a ground-up redesign. The implementation should group related fixes so each cluster improves one invariant at a time.

## Approach

### 1. Bound retransmission starvation without removing priority

- Keep audio first.
- Introduce a bounded mixed-media pacing rule when retransmission and fresh video are both queued:
  - retransmission keeps first opportunity
  - fresh video receives an explicit periodic send turn instead of waiting for the retransmission queue to empty
- Preserve shutdown draining behavior and queue accounting.
- Update accepted send-side BWE behavior and tests to match the new fairness invariant.

### 2. Narrow registry maintenance critical sections

- Make `registerNode()` and `evictDeadNodes()` own their own command mutex usage instead of relying on one outer `heartbeat()` lock.
- In `evictDeadNodes()`, fail fast if any `appendCommand()` call fails and disconnect the command context before consuming replies.
- Keep cache mutation outside of unrelated broader command phases.

This is a focused maintenance-path correction, not a full registry executor redesign.

### 3. Standardize FFmpeg defensive checks

- Add explicit null guards to:
  - `BitstreamFilter::Create()`
  - `Decoder::ReceiveFrame()`
  - `Encoder::ReceivePacket()`
- Reuse the existing wrapper style: fail fast with `std::runtime_error`.
- Extend shared FFmpeg tests to cover misuse symmetry.

### 4. Make lifecycle cleanup explicit

- `EventEmitter`
  - maintain an ID-to-event index so `off(id)` does not scan all events
  - add an internal checked emit path for cleanup-critical events so exceptions are not silently hidden
- Producer / Consumer / Transport / Router
  - standardize internal `@close` as the terminal-close signal for all close reasons
  - pass a reason token in the internal close event so close semantics are explicit
  - keep existing public events (`transportclose`, `producerclose`, `routerclose`, `workerclose`) for user-facing behavior
- Channel / listener ownership
  - store direct channel-listener IDs on Producer / Consumer / Transport and unregister by ID
  - avoid broad `off(eventName)` cleanup where a direct owned handle is available
- Transport cleanup
  - consolidate duplicated `close()` / `routerClosed()` cleanup into a shared helper
- API misuse prevention
  - delete copy / move for Producer and Consumer
  - avoid request ID collision on wrap-around by scanning for an unused pending ID before inserting

### 5. Close remaining operational/documentation gaps

- `RoomRegistryPubSub.cpp`
  - provide a deterministic non-zero jitter fallback when `/dev/urandom` is unavailable
- `Recorder`
  - add explicit warning / counters when pending audio overflows during deferred-header mode
- `RtcpHandler.h`
  - derive NTP time from a stable monotonic elapsed clock anchored to an initial wall-clock base
- `QosController`
  - add override housekeeping that can run even when sample flow is paused, and invoke it from the plain-client control loops
- `SignalingServer.h`
  - document `destroyedRooms_` thread affinity explicitly beside `roomDispatch_`
- specs
  - document accepted worker-crash behavior: `serverRestart` + room teardown, not lossless room recovery
  - document the strengthened pacing / registry / lifecycle invariants

## Non-Goals

- introducing a fully formal lifecycle state machine object model
- redesigning Worker respawn to preserve existing rooms after crash
- replacing `EventEmitter` with a new external dependency

## Risks

- pacing fairness changes can perturb current bitrate-distribution tests
- checked cleanup emits may surface listener bugs that were previously hidden
- listener-ID ownership changes must not leave stale notification subscriptions behind
- registry locking changes must preserve single-context hiredis safety

## Verification Strategy

- unit tests for FFmpeg null guards and EventEmitter cleanup behavior
- sender-transport tests updated to prove retransmission priority plus fresh-video progress
- registry maintenance tests for heartbeat/eviction and partial-failure behavior
- recorder / RTCP / QoS targeted tests for the new operational guardrails
- targeted integration tests for worker-crash accepted behavior remain green
- build:
  - `mediasoup_tests`
  - `mediasoup_qos_unit_tests`
  - `mediasoup_review_fix_tests`
  - `mediasoup-sfu`

## Rollout Notes

- The riskiest user-visible change is pacing fairness under retransmission backlog; that must ship with spec/test updates.
- Lifecycle cleanup becomes stricter and more explicit; failures that were previously only logged may now surface during cleanup-critical internal events.
