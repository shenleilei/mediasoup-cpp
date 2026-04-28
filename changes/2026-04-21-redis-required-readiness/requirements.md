# Redis Required Readiness

## Problem Statement

The current runtime contract allows the SFU to start and continue admitting new rooms when Redis-backed room registry functionality is unavailable. That behavior conflicts with the intended multi-node routing model because room ownership and resolve decisions depend on Redis coordination.

## Business Goal

Make Redis a strong runtime dependency for clustered signaling behavior so the process does not advertise readiness or accept new room-routing decisions when Redis is unavailable.

## In Scope

- Treat Redis-backed room registry availability as a readiness requirement.
- Fail startup when Redis is unavailable.
- Expose Redis readiness clearly in health and metrics surfaces.
- Reject new routing-dependent requests when Redis becomes unavailable after startup.
- Update tests and docs to match the new contract.

## Out Of Scope

- Redis high availability features such as Sentinel, clustering, or connection pools.
- Reworking existing media sessions during a runtime Redis outage.
- Changing GeoRouter behavior beyond readiness reporting context.

## User Stories Or Scenarios

- As an operator, when Redis is down during startup, the SFU process should fail startup instead of silently serving local-only traffic.
- As an operator, when Redis disconnects after startup, readiness checks should fail so load balancers and monitoring can drain the node.
- As an operator, I need metrics and HTTP status surfaces that clearly show whether Redis is ready.
- As a caller, routing-dependent APIs should not silently degrade to local-only behavior when Redis is unavailable.

## Acceptance Criteria

- Startup SHALL fail when the configured Redis registry cannot be initialized.
- `/healthz` SHALL continue to report process liveness and worker state.
- `/readyz` SHALL exist and SHALL return `503` when Redis is unavailable or workers are unavailable.
- Runtime metrics SHALL expose whether Redis is required and whether Redis is currently ready.
- New join and resolve flows SHALL NOT silently degrade to local-only behavior when Redis is unavailable; they SHALL return an explicit failure.
- Existing tests that encoded local-only Redis fallback SHALL be updated to assert the new contract.

## Non-Functional Requirements

- Readiness evaluation SHALL be cheap and safe to call frequently.
- Runtime Redis disconnect detection SHALL avoid null dereference or race-prone access to registry state.
- Operator-facing logs for Redis readiness loss SHALL remain explicit without flooding normal paths.

## Edge Cases And Failure Cases

- Redis unavailable during startup.
- Redis command connection lost after startup.
- Redis subscriber connection lost while command connection remains healthy.
- Worker pool healthy but Redis unhealthy.
- Redis healthy but worker pool unavailable.

## Open Questions

- None. This change assumes the clustered runtime contract requires Redis by default and no longer permits implicit single-node fallback.
