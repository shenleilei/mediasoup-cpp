# Bugfix Analysis

## Summary

The current plain-client sender QoS control path still mixes:

- local RTCP / transport observations
- server `getStats`
- optional test injection

inside one control snapshot path.

That violates the rewrite direction already accepted in the planning docs: sender QoS control inputs should come only from local transport truth.

This wave removes server `getStats` from the sender QoS control chain and deletes the attached compatibility logic:

- `RequestServerProducerStats()`
- cached server stats response state
- server producer stats parsing for control snapshots
- `LossCounterSource`
- per-track `lossBase`

## Reproduction

1. Inspect `PlainClientLegacy.cpp` and `PlainClientThreaded.cpp`.
2. Observe that both paths request `getStats` from the server before sender QoS sampling.
3. Observe that server stats can overwrite:
   - `bytesSent`
   - `packetsSent`
   - `packetsLost`
   - RTT / jitter
4. Observe that additional boundary logic exists only to make that source mixing survivable.

## Observed Behavior

- Sender QoS control inputs are not sourced from one canonical local transport path.
- Legacy and threaded implementations both contain source-mixing logic.
- Per-track loss baselining exists only because server stats are being reinterpreted as local sender control input.

## Expected Behavior

- Sender QoS control snapshots SHALL use only local transport-path data.
- Server `getStats` SHALL not overwrite sender QoS control fields.
- Source-mixing compensation logic tied to server stats SHALL be removed.

## Known Scope

- `client/PlainClientApp.h`
- `client/PlainClientApp.cpp`
- `client/PlainClientLegacy.cpp`
- `client/PlainClientThreaded.cpp`
- `client/PlainClientSupport.h`
- `client/PlainClientSupport.cpp`
- `client/ThreadedControlHelpers.h`

## Must Not Regress

- local transport/TWCC/RTCP sender QoS sampling
- client-side snapshot emission to the server
- matrix/synthetic profile test injection support
- QoS policy / override handling

## Suspected Root Cause

High confidence:

- server stats were originally mixed into sender QoS as a pragmatic supplement
- additional fields and baseline logic accumulated around that design
- the architecture no longer matches the intended “local transport truth” model

## Acceptance Criteria

### Requirement 1

The plain-client sender QoS control chain SHALL no longer depend on server `getStats`.

#### Scenario: legacy or threaded sender QoS sampling

- WHEN sender QoS samples are built
- THEN control fields are populated only from local transport observations and test-only injection paths
- AND server `getStats` is not requested or parsed for control snapshots

### Requirement 2

The wave SHALL delete server-stats-specific compensation logic.

#### Scenario: source-mixing cleanup

- WHEN this wave is complete
- THEN `lossBase`, `LossCounterSource`, and cached server-stats control state are removed from the sender QoS runtime path

## Regression Expectations

- Required automated verification:
  - targeted build of affected native targets
  - targeted tests that still cover sender QoS helper/state logic where feasible
- Required manual smoke checks:
  - inspect trace output paths to confirm no `sample=server` control path remains
