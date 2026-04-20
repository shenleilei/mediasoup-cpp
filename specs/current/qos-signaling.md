# QoS Signaling

## Worker-Completed Stats Responses

- `clientStats` and `downlinkClientStats` SHALL send their response only after the owning `WorkerThread` has processed the request far enough to determine the store result.
- A successful QoS stats response SHALL include `data.stored`.
- When the worker-side registry intentionally ignores a structurally valid snapshot, the response SHALL remain `ok=true` and SHALL include `data.stored=false` plus `data.reason`.
- When runtime preconditions disappear before worker-side processing completes, the response SHALL fail instead of reporting success.

## Store Outcome Semantics

- `data.stored=true` means the snapshot was accepted into the worker-side QoS registry.
- `data.stored=false` means the request was processed but the snapshot was not stored, for example because it was stale relative to the existing registry entry.
