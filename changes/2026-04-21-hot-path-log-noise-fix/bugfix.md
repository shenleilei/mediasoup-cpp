# Bugfix: Hot-path warning log noise

## Symptom

Normal request and registry maintenance flow emits `warn`-level logs on hot paths.

## Reproduction

1. Run the SFU with `/api/resolve` traffic or normal registry heartbeat/load updates.
2. Observe warning logs filling with expected request lifecycle traces.

## Observed behavior

- `/api/resolve` request receipt/execution/resolution/response scheduling logs at warning level.
- `RoomRegistry` heartbeat, updateLoad, refreshRooms, and resolveRoom normal path tracing logs at warning level.

## Expected behavior

- Expected hot-path tracing should log below warning level.
- Actual failures, degradation, or anomalous states should continue to log at warning level.

## Suspected scope

- `src/SignalingServerHttp.cpp`
- `src/RoomRegistry.cpp`
- `specs/current/runtime-safety.md`

## Known non-affected behavior

- Existing warning logs for Redis failures, stale redirect cleanup, parse failures, and unexpected claim results remain warnings.

## Acceptance criteria

- Normal `/api/resolve` lifecycle tracing no longer emits warning logs.
- Normal `RoomRegistry` heartbeat/updateLoad/refreshRooms/resolveRoom tracing no longer emits warning logs.
- Failure/degradation warning logs remain unchanged.
- Build and relevant unit suites pass.

## Regression expectations

- No behavior change beyond log severity for normal paths.
