# Bugfix Analysis

## Summary

After wave 1 and wave 2:

- sender QoS control snapshots are local-only
- the control path uses `CanonicalTransportSnapshot`

The remaining architectural mismatch is runtime entry ownership:

- legacy path still contains sender QoS runtime logic
- threaded path is intended to become the sole sender QoS runtime path

This wave makes threaded mode the only sender QoS runtime entry and deletes legacy sender QoS runtime logic.

## Reproduction

1. Inspect `PlainClientApp::ConfigureQosControllers()`.
2. Observe that QoS controllers are still created independently of `threadedMode_`.
3. Inspect `RunLegacyMode()` and observe that it still contains sender QoS sampling, peer coordination, and client snapshot upload logic.

## Observed Behavior

- legacy path still carries sender QoS runtime code
- threaded path is not yet the sole runtime entry for sender QoS

## Expected Behavior

- sender QoS controllers SHALL only exist in threaded mode
- legacy mode SHALL no longer run sender QoS sampling/coordination/reporting logic

## Known Scope

- `client/PlainClientApp.cpp`
- `client/PlainClientApp.h`
- `client/PlainClientLegacy.cpp`

## Must Not Regress

- legacy plain media send path without sender QoS
- threaded sender QoS runtime path
- policy / override handling in threaded mode

## Acceptance Criteria

### Requirement 1

Threaded mode SHALL become the only sender QoS runtime path.

#### Scenario: plain-client startup

- WHEN plain-client starts in non-threaded mode
- THEN sender QoS controllers are not instantiated
- AND legacy runtime does not execute sender QoS control logic

## Regression Expectations

- Required automated verification:
  - build affected targets
  - run targeted sender QoS unit tests
- Required manual smoke checks:
  - inspect legacy runtime path for removed sender QoS control code
