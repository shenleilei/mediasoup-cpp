# Bugfix Analysis

## Summary

After wave 3:

- sender QoS controllers only exist on the threaded runtime path
- non-copy plain-client execution already routes into `RunThreadedMode()`

The remaining mismatch is naming and operational residue:

- `PlainClientApp` still stores `threadedMode_` even though it is derived from `!copyMode_`
- setup/examples/tests still advertise `PLAIN_CLIENT_THREADED=1`
- harness coverage still expects a "threaded fallback to legacy" warning that is no longer part of the target architecture
- session bootstrap still creates transitional QoS controllers and dispatches queued notifications before the final threaded runtime controllers exist

This wave removes those obsolete entry semantics so the repo reflects the final runtime model instead of a transitional one.

## Reproduction

1. Search the repo for `PLAIN_CLIENT_THREADED`.
2. Inspect `PlainClientApp::ParseArguments()`.
3. Inspect `InitializeSession()` and observe that queued notifications are dispatched before `RunThreadedMode()` creates the final threaded controllers.
4. Inspect threaded harness scenarios and setup examples.

## Observed Behavior

- runtime state still keeps a redundant `threadedMode_` flag
- operational docs still say threaded mode must be explicitly enabled
- one harness scenario still validates legacy fallback rather than the new default threaded execution contract
- early `qosPolicy` / `qosOverride` notifications can be consumed by transitional controllers that are replaced at threaded runtime startup

## Expected Behavior

- non-copy plain-client execution SHALL be treated as the default threaded runtime without an extra opt-in flag
- repo docs, setup commands, tests, and harnesses SHALL stop referencing the removed threaded env toggle
- harness coverage SHALL validate the new default threaded multi-track behavior instead of a removed fallback path
- queued QoS notifications SHALL first hit the final threaded runtime controllers rather than a bootstrap-only controller instance

## Known Scope

- `client/PlainClientApp.*`
- `setup.sh`
- `tests/qos_harness/*`
- `tests/test_thread_integration.cpp`
- `docs/qos/plain-client/*`

## Must Not Regress

- copy-mode plain media send path
- default non-copy threaded sender QoS runtime
- threaded multi-track harness and TWCC evaluation entrypoints

## Acceptance Criteria

### Requirement 1

The runtime SHALL no longer expose obsolete threaded opt-in state.

#### Scenario: plain-client startup

- WHEN plain-client starts without `--copy`
- THEN sender QoS runtime setup follows the threaded path directly
- AND no extra threaded env toggle is required or documented

### Requirement 2

Regression and operator surfaces SHALL match the new runtime contract.

#### Scenario: harness and setup usage

- WHEN developers read setup examples or run threaded harness scenarios
- THEN they do not need `PLAIN_CLIENT_THREADED=1`
- AND regression scenarios do not expect a legacy fallback warning

## Regression Expectations

- Required automated verification:
  - build affected native targets
  - run targeted sender QoS unit tests
  - run JS syntax checks for edited harness scripts
- Required review checks:
  - confirm no remaining repo-local `PLAIN_CLIENT_THREADED` references on the supported path
