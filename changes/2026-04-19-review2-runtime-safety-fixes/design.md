# Design

## Goal

Land the high-confidence `revew2.md` runtime safety fixes without broad architectural refactors.

## Approach

### 1. WebSocket message hardening

Wrap the message callback in exception handling and validate the top-level `request` flag before reading typed fields.

This keeps malformed client payloads on the error path instead of letting `nlohmann::json` exceptions escape the uWS callback.

### 2. Explicit room-registry comparator helpers

Extract the no-geo and geo candidate ordering logic into a small helper header.

This allows:

- the no-geo comparator to use a proper self tie-breaker
- direct unit coverage of the comparator contract
- reuse in `RoomRegistry::findBestNodeCached()` without duplicating the ordering logic

### 3. Worker shutdown synchronization

Use `std::atomic<bool>` for `Worker::closed_` and guard `routers_` with a mutex.

Close and death paths will:

- gate on `closed_.exchange(true)`
- snapshot and clear routers under the mutex
- perform `router->workerClosed()` outside the lock

This removes concurrent mutation of `routers_` and makes close/death idempotent.

### 4. Channel self-thread close safety

When `Channel::close()` runs on the read thread, detach the thread before destruction instead of leaving it joinable.

This preserves the existing behavior while removing the `std::terminate` hazard on object destruction.

### 5. Recorder defensive checks

Add narrow guardrails in `PeerRecorder`:

- use an atomic socket fd for start/stop/recv-loop coordination
- move `outputPath_` checks under `qosMutex_`
- fail safely if `avformat_new_stream()` returns null
- reject H264 extradata construction when SPS is shorter than 4 bytes

These changes are local and do not alter the recorder data model or muxing flow.

## Verification

- integration regression for malformed WebSocket requests
- unit regression for no-geo comparator ordering
- recorder unit regression for short SPS rejection
- targeted build/tests for integration, unit, QoS unit, and threaded/runtime suites
