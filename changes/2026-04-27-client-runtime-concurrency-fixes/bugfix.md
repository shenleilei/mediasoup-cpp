# Bugfix Plan: Client Runtime Concurrency Fixes

## Symptom
Several client/runtime support components still contain concurrency hazards that can corrupt state or rely on undefined behavior under legitimate multi-threaded use.

## Reproduction
1. Review `client/ccutils/Prober.h` and observe `stopRequested_` written outside the mutex discipline used by `Run()`.
2. Review `client/NetworkThread.h` and observe probe packet sequence mutation through `const_cast`.
3. Review `client/WsClient.*` and observe:
   - `recvText()` remains public even though `readerLoop()` already owns the read side
   - fd lifecycle is not protected by a single coherent close/send/recv synchronization model

## Observed Behavior
- `Prober::Reset()` can race with concurrent `AddCluster()` / worker lifecycle decisions.
- `NetworkThread::SendProbePacket()` mutates `TrackNetState::seq` through a `const TrackNetState&`.
- `WsClient` exposes an external read primitive that can race with the internal reader thread and also lacks a fully serialized fd lifecycle during close.

## Expected Behavior
- Probe worker lifecycle SHALL be managed by a single well-defined synchronized state model.
- Probe send paths SHALL mutate track state only through non-const references/pointers.
- `WsClient` SHALL have a single owner for inbound reads and a thread-safe fd lifecycle for send, shutdown, join, and close.

## Suspected Scope
- `client/ccutils/Prober.h`
- `client/NetworkThread.h`
- `client/WsClient.h`
- `client/WsClient.cpp`
- Related unit tests under `tests/`

## Acceptance Criteria
- `Prober` reset/stop/add-cluster logic no longer performs lockless access to worker lifecycle flags and cannot strand queued clusters during reset.
- `NetworkThread` probe sending no longer uses `const_cast` to mutate probe sequence state.
- `WsClient::recvText()` is not externally callable while the internal reader thread owns inbound reads.
- `WsClient` close/send/reader interactions are synchronized so fd shutdown/close is idempotent and does not race unsafely with active operations.
- Targeted unit coverage exists for the repaired runtime behavior where practical.

## Regression Expectations
- Existing QoS / threaded runtime regressions continue to pass after the concurrency fixes.
