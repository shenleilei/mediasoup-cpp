# Design

## Context
The two remaining failures are related, but they fail at different layers:

1. Downlink startup failure:
   - browser harness reports no artificial throttle
   - receiver transport stats initially expose `availableIncomingBitrate=0`
   - the server allocator interprets that as real zero capacity and pauses visible consumers

2. Multi-track budget failure:
   - the runtime intends to coordinate multi-track camera budgets with per-track weights
   - the regression harness currently mixes local controller caps, asynchronous coordination, and server-side stats
   - threaded stats currently do not yield trustworthy per-track server-side observations for all tracks in the scenario

Both problems require tightening the runtime contract and the verification path instead of weakening the tests blindly.

## Root Cause

### 1. Downlink zero-bitrate startup ambiguity
`SubscriberBudgetAllocator` currently treats `availableIncomingBitrate == 0` and `seq > 0` as a hard zero-budget signal. During browser startup this is too aggressive because:

- the transport candidate-pair bitrate may still be unavailable
- the visible consumers may not yet have meaningful receiver-side evidence
- the health monitor has not necessarily observed genuine congestion

That converts “unknown startup bitrate” into “pause everything now”.

### 2. Multi-track verification and stats inconsistency
The multi-track runtime is supposed to:

- assign per-track weighted caps
- expose those caps to each track controller
- emit per-track client stats that reflect real send activity

The current regression evidence is not trustworthy enough:

- legacy local trace checks compare per-track local caps from asynchronous controller loops
- threaded server-side checks show only the first track with non-zero `sendBitrateBps`, which indicates that the runtime stats / snapshot path for additional tracks is incomplete or stale

The bug is therefore not only in the assertion. The runtime observation path must be corrected so the regression test validates real supported behavior.

## Approach

### A. Downlink startup budget hardening
Refine downlink budget computation so a startup `availableIncomingBitrate=0` sample is not treated as a hard zero budget unless the system has corroborating evidence that capacity is genuinely collapsed.

Practical rule for this fix:
- preserve the existing explicit-budget behavior for positive bitrate samples
- preserve degraded behavior when the planner is already in a degraded state
- avoid immediate visible-consumer pause when zero bitrate is only an initial unknown sample

This keeps the allocator conservative without allowing a startup stats hole to pause healthy calls.

### B. Multi-track runtime stats correctness
Trace the threaded multi-track path from:

- per-track runtime stats sampling
- peer snapshot assembly
- server-side `clientStats` storage

and fix the point where additional tracks lose usable `sendBitrateBps` observations.

The fix must ensure:
- every published video track contributes stable per-track stats
- weighted coordination decisions are based on those stats
- server-side `clientStats` exposes all relevant tracks with meaningful per-track bitrate signals

### C. Multi-track regression assertion cleanup
Keep the regression focused on supported guarantees:

- all expected tracks are present
- the sacrificial lowest-weight track stays lowest under constrained budget
- higher-weight tracks remain above that sacrificial track over a bounded observation window

Use synchronized, trustworthy observations:
- threaded/server-side `clientStats` and/or runtime stats after the stats bug is fixed
- avoid relying on a single last local cap sample from an asynchronous loop when that sample is not a true transport observation

## Non-goals
- Rewriting the entire downlink allocator architecture.
- Introducing a brand-new generalized media scheduler beyond the current supported scope.
- Weakening tests to hide runtime accounting bugs.

## Verification Strategy
- Add or update focused C++ downlink allocator tests for startup zero-bitrate handling.
- Re-run `scripts/run_qos_tests.sh browser-harness:downlink-priority`.
- Re-run `scripts/run_qos_tests.sh cpp-client-harness:multi_video_budget`.
- Re-run `node tests/qos_harness/run_cpp_client_harness.mjs threaded_multi_video_budget`.
- If supported behavior changes materially, update the accepted spec and rerun the relevant QoS regression entry.
