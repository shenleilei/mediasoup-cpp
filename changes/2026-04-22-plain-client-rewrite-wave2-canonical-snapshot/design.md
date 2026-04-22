# Bugfix Design

## Context

Wave 1 removed server stats from sender QoS control sampling.

Wave 2 should make that architectural change visible in the code model:

- the sender control input is no longer a raw/mixed snapshot
- it is the canonical transport snapshot produced from local transport truth

## Root Cause

The runtime control path still uses:

- `RawSenderSnapshot`

across:

- type definitions
- controller entrypoints
- signal derivation
- snapshot serialization helpers

That naming keeps the old mental model alive even after the source-mixing removal.

## Fix Strategy

### 1. Replace the control input type name

Rename the sender QoS control input type from:

- `RawSenderSnapshot`

to:

- `CanonicalTransportSnapshot`

### 2. Update all direct runtime consumers

Update:

- signal derivation
- controller state
- snapshot serialization helpers
- plain-client producer paths
- affected tests

### 3. Keep semantics stable

This wave changes naming and architectural clarity, not behavior.

The snapshot still carries the fields the control path currently needs, but the type now encodes the correct design intent.

## Risk Assessment

- Renaming a central type touches many files and tests.
- If any runtime call site is missed, compile failures should catch it quickly.

## Test Strategy

- build `plain-client`
- build `mediasoup_qos_unit_tests`
- run targeted sender QoS unit tests

## Observability

- not applicable beyond consistent trace/source labeling already cleaned up in wave 1

## Rollout Notes

- This is a direct follow-up to wave 1 and should land before deeper controller/signals refactors so the rest of the rewrite builds on the correct vocabulary.
