# Bugfix Analysis

## Summary

After removing server stats from the sender QoS control chain, the next remaining architectural issue is that the control pipeline still revolves around a type named `RawSenderSnapshot`.

That name no longer matches the intended design:

- the control path should consume one canonical transport truth object
- not an arbitrary raw/mixed snapshot assembled from multiple sources

This wave renames and re-anchors the sender QoS control input around a canonical transport snapshot so the type system reflects the new architecture.

## Reproduction

1. Inspect current sender QoS types and controller entrypoints.
2. Observe that the control path still uses `RawSenderSnapshot`.
3. Observe that the new rewrite direction expects one canonical transport snapshot sourced from local transport truth.

## Observed Behavior

- the control input type still implies a generic/raw snapshot
- the type name does not reinforce the local-transport-truth architecture
- supporting helpers and tests still speak in old vocabulary

## Expected Behavior

- the sender QoS control input type SHALL reflect canonical local transport truth
- controllers, signal derivation, and snapshot serialization SHALL use that canonical type

## Known Scope

- `client/qos/QosTypes.h`
- `client/qos/QosSignals.h`
- `client/qos/QosController.h`
- `client/qos/QosProtocol.h`
- `client/PlainClientSupport.*`
- `client/PlainClientLegacy.cpp`
- `client/PlainClientThreaded.cpp`
- related tests

## Must Not Regress

- sender QoS behavior from wave 1
- client snapshot emission to the server
- matrix/synthetic test injection support

## Suspected Root Cause

High confidence:

- the original control path evolved before the current rewrite direction was made explicit
- the type name and its surrounding call sites still reflect the older “raw snapshot” mindset

## Acceptance Criteria

### Requirement 1

The sender QoS control path SHALL use a canonical transport snapshot type.

#### Scenario: sender QoS sample enters the controller

- WHEN sender QoS builds a control sample
- THEN the sample type name and downstream interfaces reflect canonical local transport truth
- AND the old `RawSenderSnapshot` name is removed from the runtime control path

## Regression Expectations

- Required automated verification:
  - build affected targets
  - run targeted QoS unit tests
- Required manual smoke checks:
  - inspect type and function names for consistent canonical-snapshot vocabulary
