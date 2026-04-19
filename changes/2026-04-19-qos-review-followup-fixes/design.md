# Bugfix Design

## Context

This change touches the publisher QoS signaling path, browser demo/debug code,
and playback rendering. The goal is to fix review findings without widening the
accepted server contracts for downlink QoS.

## Root Cause

### 1. Duplicate publisher snapshot parsing

`SignalingServerWs` already validates `clientStats` before posting work to a
`WorkerThread`, but it forwards the raw JSON payload. `RoomService::setClientStats()`
then parses the same payload again.

### 2. Legacy demo path drifted from the accepted schema

The demo legacy fallback predates the strict
`mediasoup.qos.client.v1` contract. It still tries to upload a partial object
that cannot pass server validation.

### 3. Demo/playback rendering assumes optional downlink fields

Older recordings and partial browser stats do not guarantee concealment/freeze
fields. The UI code uses arithmetic directly on optional properties.

### 4. Browser debug sampling only captures a subset of available downlink data

`collectLocalDebugStats()` reads a narrow set of `inbound-rtp` metrics and does
so through mediasoup-client private internals.

## Fix Strategy

### 1. Move parsed `clientStats` across the worker boundary

- Change the worker handoff to capture `qos::ClientQosSnapshot`
- Update `RoomService::setClientStats()` to accept the parsed snapshot directly
- Keep all existing server-side registry/aggregation logic unchanged

### 2. Make legacy demo mode explicit and non-disruptive

- Stop sending invalid legacy `clientStats` uploads
- Keep local browser debug polling active so the demo still provides useful
  visibility
- Surface a clear log/status message so the user understands the limitation

### 3. Harden downlink rendering for optional fields

- Add numeric guards before concealment/freeze calculations
- Make playback prefer a server-provided producer `clockRate` when available and
  fall back to the previous heuristic only when necessary

### 4. Enrich browser-side local debug sampling using public APIs

- Replace `_handler._pc.getStats()` calls with `Transport.getStats()`
- Capture audio concealment counters when browsers expose them
- Capture freeze-related counters from inbound video stats for local rendering
- Track packet-loss deltas for local debug summaries instead of displaying raw
  monotonically increasing counters as if they were interval metrics

## Risk Assessment

- The worker handoff change is low risk because `QosRegistry` already stores the
  parsed snapshot type.
- The legacy-mode change intentionally preserves strict server validation rather
  than widening the schema.
- UI/debug sampling changes are isolated to `public/` and do not alter the
  server-side downlink contract.
- Producer `clockRate` exposure slightly expands the stats payload, but it is a
  backward-compatible additive field.

## Test Strategy

- Reproduction test:
  - targeted C++ QoS tests covering stale/reset seq handling and `clientStats`
    integration
- Regression test:
  - JS unit coverage for downlink sampler delta/optional-field handling
- Adjacent-path checks:
  - playback/demo rendering path stays safe with missing optional fields
- Integration test if the bug crosses a module or service boundary:
  - `clientStats` signaling/integration tests remain green after the parsed
    handoff change

## Observability

- Existing `invalid clientStats` rejection logs remain intact for truly invalid
  payloads
- Demo log messaging becomes explicit for legacy mode instead of hiding repeated
  upload failures

## Rollout Notes

- No migration is required
- Rollback is straightforward because all changes are local and additive
