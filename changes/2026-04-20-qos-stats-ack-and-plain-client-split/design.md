# Design

## Context

The current QoS stats request path conflates "posted to the worker queue" with "accepted by the runtime". That is the wrong observability boundary for a control-plane API because:

- queueing success is not business success
- the worker may discover that the room/peer is already gone
- the registry may intentionally ignore the snapshot as stale
- exceptions after `post(...)` are invisible to the caller

At the same time, `downlinkQos` itself is still an async planner output and should stay that way.

On the client side, `client/main.cpp` already shed RTCP/network/source-worker helpers into separate headers, but it still owns:

- the raw WebSocket protocol client
- server-stats parsing helpers used by QoS sampling
- matrix/test-env parsing helpers
- the main orchestration loop

That file is still too broad.

## Proposed Approach

### 1. Worker-completed QoS stats responses

Change `clientStats` and `downlinkClientStats` handling so the response is deferred until the worker thread finishes the store/ignore decision.

Worker-side methods will return a structured result:

- `ok=false` when runtime preconditions fail or an exception occurs
- `ok=true, data.stored=true` when the snapshot is accepted and stored
- `ok=true, data.stored=false, data.reason=<rejectReason>` when the registry intentionally ignores the snapshot (for example `stale-seq` / `stale-ts`)

This preserves backward compatibility for callers that only inspect `ok`, while exposing the real outcome to diagnostics and tests.

### 2. Keep downlink planning asynchronous

Do not move `downlinkQos` convergence into the request-response path.

The stored snapshot is the synchronous contract boundary. Planner completion remains async and continues to feed `downlinkQos` later.

### 3. Fix rate-limit bookkeeping to follow real acceptance

The current downlink rate-limit state speculatively advances accepted sequence state before the worker result is known.

Adjust it so:

- pending/in-flight state can still throttle bursts
- accepted sequence state advances only after worker-side storage succeeds
- worker-side rejection clears stale pending state when needed

This prevents false "accepted" sequence advancement after runtime failure.

### 4. Narrow `client/main.cpp`

Extract focused plain-client modules without changing the runtime model:

- `client/WsClient.*`
  - RFC6455 handshake/frame logic
  - request/response matching
  - async notification queueing
- one additional helper module for plain-client support code
  - server-stats parsing and/or harness/env helper parsing used by the QoS/control loop

`client/main.cpp` stays the executable owner of the publish/control loop, but no longer embeds every support concern inline.

## Alternatives Considered

### Keep immediate `ok=true` and only add logs

Rejected because logs do not fix the API-level observability bug seen by callers and tests.

### Turn stale registry rejections into hard request failures

Rejected for this wave because it would widen the external behavior change more than necessary and would break current harness expectations. Additive `stored/reason` fields provide the needed observability with lower blast radius.

### Rewrite the plain client around a new runtime abstraction

Rejected because the review concern is module responsibility, not a full runtime redesign.

## Module Boundaries

- WebSocket edge/worker rendezvous remains in `src/SignalingServerWs.cpp`
- worker-side QoS store/ignore decision remains in `src/RoomServiceStats.cpp`
- plain-client WebSocket protocol code moves out of `client/main.cpp`
- additional plain-client helper logic moves out of `client/main.cpp` into a focused support module

## Failure Handling

- runtime disappearance after queueing returns `ok=false` with an explicit error
- registry ignore paths return `ok=true` plus `stored=false` and a reason
- worker exceptions are surfaced as request failures instead of disappearing behind an earlier success response
- WebSocket close still fails all pending plain-client requests after extraction

## Testing Strategy

- targeted QoS integration tests covering:
  - stored=true on accepted snapshots
  - stored=false with reason on stale client/downlink snapshots
  - preserved eventual consistency of `downlinkQos`
- targeted plain-client build after module extraction
- if practical, keep or add narrow tests around extracted helper behavior rather than broad runtime rewrites

## Rollout Notes

- response payload additions are backward-compatible
- no migration is required for plain-client CLI users
