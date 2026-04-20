# Design

## Summary

Remove the implicit local-only Redis fallback from the default runtime contract. Startup will fail if Redis-backed `RoomRegistry` cannot be created. After startup, readiness and request admission will depend on live Redis connectivity rather than silently degrading to local handling.

## Why This Approach

The current code allows three inconsistent states:

- startup succeeds without `RoomRegistry`
- `/healthz` still reports healthy even when registry is absent
- `join` and `resolve` silently degrade to local-only behavior on Redis failure

That combination violates clustered room ownership guarantees. Tightening startup, readiness, and request admission together removes the split-brain path instead of only changing monitoring cosmetics.

## Behavior Changes

### Startup

- `CreateRuntimeServices()` will treat Redis initialization failure as fatal.
- `main()` will exit through the normal startup failure path if Redis cannot be initialized.

### Runtime Readiness

- `SignalingServer::RuntimeSnapshot` will include Redis readiness fields.
- `/healthz` will remain a liveness-style endpoint and will include Redis state in the payload.
- `/readyz` will be added and will return `503` if:
  - startup did not succeed
  - shutdown is requested
  - no workers are available
  - Redis is required but not currently ready

### Metrics

- Existing `mediasoup_sfu_registry_enabled` will remain.
- New gauges will expose Redis requirement and Redis readiness.
- A top-level readiness gauge will reflect the same logic used by `/readyz`.

### Request Admission

- `RoomRegistry::claimRoom()` will stop returning local success on Redis outage.
- `RoomService::join()` will stop swallowing registry failures into local room creation.
- `RoomService::resolveRoom()` and `/api/resolve` will return explicit failure when Redis is unavailable instead of local fallback.

## Implementation Notes

### Registry Availability API

Add a lightweight `isReady()` accessor on `RoomRegistry` backed by command connection state. This keeps readiness checks centralized and avoids duplicating hiredis context inspection outside the registry.

### HTTP Contract

- `/healthz`: keep current compatibility as liveness-oriented, but include `redisRequired` and `redisReady`.
- `/readyz`: new endpoint for traffic admission and alerting.
- `/api/resolve`: when Redis is unavailable, return `503` with an explicit error instead of `{"wsUrl":"","isNew":true}`.

### WebSocket Join Contract

`join` will return `ok=false` with an explicit Redis readiness error when room claim cannot be completed because Redis is unavailable.

## Risks

- Existing tests and local workflows that assumed implicit no-Redis mode will break and must be updated.
- Operators currently using `/healthz` as readiness may need to move traffic gating to `/readyz`.

## Verification Strategy

- Unit or focused integration coverage for startup failure without Redis.
- HTTP tests for `/healthz`, `/readyz`, and `/api/resolve` Redis-unavailable behavior.
- WebSocket join test asserting Redis-unavailable rejection.
- Existing healthy-path multinode tests remain passing with temporary Redis.
