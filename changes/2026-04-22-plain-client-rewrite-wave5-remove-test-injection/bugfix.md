# Bugfix Analysis

## Summary

After wave 4:

- sender QoS control inputs are local-only
- non-copy execution defaults to the threaded runtime

The remaining architectural mismatch is test injection still living inside the production plain-client process:

- `QOS_TEST_MATRIX_PROFILE` mutates sender QoS samples inside `RunThreadedMode()`
- `QOS_TEST_CLIENT_STATS_PAYLOADS` and `QOS_TEST_SELF_REQUESTS` create in-process helper threads that issue synthetic signaling traffic
- `QOS_TEST_FORCE_STALE_TRACK_INDEX` mutates local stats freshness inside the production control loop
- `QOS_TEST_REJECT_FIRST_SET_ENCODING_TRACK_INDEX` injects command rejection inside `SourceWorker`

This wave removes those hooks from production runtime code and narrows the plain-client harness surface to externally driven scenarios only.

## Reproduction

1. Search the client runtime for `QOS_TEST_`.
2. Inspect `PlainClientThreaded.cpp` and observe synthetic matrix profile application inside sender sampling.
3. Inspect `PlainClientApp.cpp` and observe helper threads that emit synthetic `clientStats` / self-requests from env payloads.
4. Inspect `SourceWorker.h` and observe test-only encoding rejection injection.

## Observed Behavior

- production sender QoS runtime still accepts test-only env hooks
- harness scenarios depend on mutating runtime state from inside the plain-client process
- the plain-client regression surface still mixes true runtime coverage with synthetic in-process test orchestration

## Expected Behavior

- production plain-client runtime SHALL not read `QOS_TEST_*` hooks for sender QoS behavior
- sender QoS samples SHALL only reflect real local transport/runtime state
- plain-client harness scenarios SHALL be externally driven or removed from the plain-client runtime surface

## Known Scope

- `client/PlainClientApp.*`
- `client/PlainClientSupport.*`
- `client/PlainClientThreaded.cpp`
- `client/SourceWorker.h`
- `tests/qos_harness/*`
- `scripts/run_qos_tests.sh`
- `docs/qos/plain-client/*`

## Must Not Regress

- default threaded sender QoS runtime
- copy-mode plain send path
- externally driven threaded regression scenarios
- matrix / TWCC scripts that already use real loopback netem

## Acceptance Criteria

### Requirement 1

Production plain-client runtime SHALL stop reading test-only env injection hooks.

#### Scenario: runtime startup

- WHEN plain-client starts in production mode
- THEN sender QoS sampling and signaling behavior do not depend on `QOS_TEST_*` env values

### Requirement 2

Plain-client regression coverage SHALL only keep externally driven runtime-valid scenarios.

#### Scenario: cpp-client harness

- WHEN the default plain-client harness group runs
- THEN remaining scenarios use real runtime behavior plus external netem / real signaling
- AND scenarios that require in-process synthetic injection are removed from that surface

## Regression Expectations

- Required automated verification:
  - build affected native targets
  - run targeted sender QoS unit tests
  - run edited JS syntax checks
  - compile threaded integration tests after removing runtime hooks
- Required review checks:
  - confirm no remaining `QOS_TEST_` references under `client/`
