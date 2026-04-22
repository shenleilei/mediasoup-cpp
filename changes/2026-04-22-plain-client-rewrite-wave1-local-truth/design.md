# Bugfix Design

## Context

This is the first implementation wave of the rewrite-first plain-client alignment plan.

The purpose of this wave is narrow and architectural:

- remove server stats as sender QoS control input
- remove the runtime state that only existed to support that mixing

This wave does not yet redesign every sender QoS type, but it does enforce one critical boundary:

- control snapshots come from local transport truth only

## Root Cause

Two runtime paths currently mix server stats into sender control snapshots:

- `PlainClientLegacy.cpp`
- `PlainClientThreaded.cpp`

That mixing created secondary logic:

- `RequestServerProducerStats()`
- cached server stats response objects
- `LossCounterSource`
- `lossBase`

The additional state is not the real feature. It is scaffolding around a source-mixed control chain.

## Fix Strategy

### 1. Remove server stats from control snapshots

Delete the following behavior from both sender QoS paths:

- request server `getStats`
- read cached server stats before control sampling
- overwrite local sender QoS fields from server producer stats

### 2. Remove obsolete source-mixing state

Delete the runtime state that only existed to support server/local mixing:

- `RequestServerProducerStats()`
- `cachedServerStatsResponse_`
- `ServerProducerStats`
- `CachedServerStatsResponse`
- `LossCounterSource`
- `lossBaseInitialized`
- `lossBase`

### 3. Keep test-only injection support

Do not remove:

- matrix/synthetic profile injection
- client snapshot emission to the server
- policy / override notifications

Those are not the same thing as server `getStats` driving local sender control.

### 4. Keep peer-level coordination, but source it from local signals

Where budget/coordination logic previously used server-derived observed bitrate or server-driven bandwidth-limited hints, derive that from the controller's local last signals instead.

## Risk Assessment

- Removing server stats may expose gaps in local sender transport truth.
- Legacy path may lose one of its previous fallback heuristics.
- Trace output wording will change and any tooling that expected `sample=server` may need to adapt.

## Test Strategy

- build the changed targets
- run targeted unit tests in `mediasoup_tests` / `mediasoup_qos_unit_tests` where applicable
- inspect trace/log source labels after the refactor if a runtime smoke check is feasible

## Observability

- keep trace output, but ensure it reflects only local or matrix-derived sender control sources

## Rollout Notes

- This wave intentionally deletes code rather than preserving fallback behavior.
- If a local transport signal proves missing after this change, fix that in the transport path rather than reintroducing server-stats-as-control-input.
