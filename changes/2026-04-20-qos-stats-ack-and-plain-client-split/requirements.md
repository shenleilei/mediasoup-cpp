# Problem Statement

Two follow-up review findings are still valid in the current tree:

- `clientStats` / `downlinkClientStats` return `ok=true` immediately after `WorkerThread::post(...)`, before the worker has actually accepted or rejected the snapshot. If the room/peer disappears, the registry drops the snapshot, or the worker task throws, the client still observes a success response with no direct signal about the real outcome.
- `client/main.cpp` still keeps WebSocket protocol code, server-stats/QoS helper code, harness/test-env parsing, and the main media/control loop in one oversized translation unit, which raises maintenance and regression-localization cost.

## Business Goal

Make QoS stats request outcomes truthful and diagnosable without breaking the accepted async downlink-planning model, and reduce `client/main.cpp` responsibility so review and future changes can target narrower modules.

## In Scope

- `src/SignalingServerWs.cpp`
- `src/SignalingSocketState.h`
- `src/RoomService.h`
- `src/RoomServiceStats.cpp`
- affected QoS integration/spec/docs files
- `client/main.cpp`
- new focused plain-client support modules extracted from `client/main.cpp`
- `client/CMakeLists.txt`
- affected plain-client docs

## Out Of Scope

- changing the accepted async nature of `downlinkQos` planning/materialization
- redesigning the plain-client runtime/thread model
- rewriting the plain client into a framework or changing its external CLI contract
- broad cleanup outside the touched QoS signaling path and extracted client modules

## Scenarios

### Scenario 1: QoS stats response reflects worker outcome

When a client sends `clientStats` or `downlinkClientStats`, the response is sent only after the owning worker has processed the request far enough to know whether the snapshot was stored, ignored as stale, or rejected because the runtime state disappeared.

### Scenario 2: Accepted async planning stays async

When a valid `downlinkClientStats` snapshot is stored, the response does not claim that `downlinkQos` has already converged; only snapshot storage becomes synchronously observable.

### Scenario 3: Plain-client module boundaries are narrower

A developer can change the plain-client WebSocket implementation or its server-stats/test-harness helper logic without editing the main media/control loop in `client/main.cpp`.

## Acceptance Criteria

1. `clientStats` and `downlinkClientStats` SHALL not send `ok=true` before the worker has processed the request.
2. A successful QoS stats response SHALL include whether the snapshot was stored, and if it was ignored by the registry, it SHALL include the reject reason.
3. If the worker-side runtime preconditions are gone by processing time, the QoS stats response SHALL fail instead of reporting success.
4. `downlinkQos` SHALL remain eventually consistent with a stored `downlinkClientStats` request; the change SHALL not make planner completion part of the request-response contract.
5. `client/main.cpp` SHALL have materially narrower responsibilities than the current baseline by extracting at least the WebSocket client implementation and one additional focused helper area into separate modules.
6. Existing plain-client CLI/runtime behavior SHALL remain compatible after the extraction.

## Non-Functional Requirements

- Preserve existing stale-session checks and room-to-worker-thread ownership rules.
- Keep QoS request handling explicit and diagnosable with narrow additive response fields instead of silent background failure.
- Keep the plain-client refactor behavior-equivalent; no intentional media/QoS/threading behavior changes beyond module boundaries.

## Edge Cases And Failure Cases

- room destroyed or peer removed after the WebSocket thread validates the request but before the worker handles it
- stale/reset QoS sequence handling that is intentionally ignored by the registry
- worker task exceptions while processing QoS stats
- downlink rate-limit bookkeeping after worker-side rejection
- plain-client async request/notification handling during close/shutdown after WebSocket extraction

## Open Questions

- Whether future callers should start asserting the new `stored` response field directly in harness code, or keep it as additive diagnostics for now
