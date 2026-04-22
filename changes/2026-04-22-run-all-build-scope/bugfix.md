# Bugfix Analysis

## Summary

`scripts/run_all_tests.sh` currently prebuilds a fixed, superset target list before it looks at the selected regression groups.

That means commands such as:

- `./scripts/run_all_tests.sh non-qos`

still build QoS/threaded-only targets such as:

- `mediasoup_thread_integration_tests`
- `plain-client`

This is wasteful on normal machines and can fail on lower-memory hosts even when the user explicitly asked for a non-QoS run.

Additionally, `mediasoup_thread_integration_tests` is a particularly heavy target built from `tests/test_thread_integration.cpp`, so even when it is legitimately needed it can trigger OOM on smaller hosts unless it is built with constrained parallelism.

## Reproduction

1. Run:
   - `./scripts/run_all_tests.sh non-qos`
2. Observe the prebuild step in `build_targets()`.
3. Observe that it still builds `mediasoup_thread_integration_tests` and `plain-client`.

## Observed Behavior

- Selected groups affect execution, but not the upfront build target list.
- `non-qos` runs can fail while compiling QoS/threaded-only binaries that are never executed in that run.

## Expected Behavior

- `run_all_tests.sh` SHALL prebuild only the native targets required by the selected groups.
- QoS/threaded-only artifacts SHALL not be prebuilt for `non-qos` runs.
- Delegated QoS paths MAY continue building their own artifacts inside `scripts/run_qos_tests.sh`.
- The default full regression entrypoint SHALL also include vendored worker regression cases.
- When `mediasoup_thread_integration_tests` or vendored worker tests must be built, the scripts SHALL use constrained parallelism.

## Known Scope

- `scripts/run_all_tests.sh`
- `scripts/run_qos_tests.sh`
- vendored worker regression invocation from `scripts/run_all_tests.sh`

## Must Not Regress

- default full-run behavior with no explicit group selection
- `qos` and `threaded` group execution
- integration/topology preflight binaries

## Suspected Root Cause

High confidence:

- `build_targets()` uses a hard-coded full target list and is called before group execution begins.
- group normalization exists, but it is not used to shape the build phase.

## Acceptance Criteria

### Requirement 1

The full regression entrypoint SHALL narrow its prebuild targets to the selected groups.

#### Scenario: `non-qos`

- WHEN the user runs `./scripts/run_all_tests.sh non-qos`
- THEN the prebuild phase does not build `mediasoup_thread_integration_tests`
- AND the prebuild phase does not build `plain-client`

### Requirement 2

The default full run SHALL remain complete.

#### Scenario: no explicit groups

- WHEN the user runs `./scripts/run_all_tests.sh`
- THEN the script still prepares all required native targets for the selected default groups
- AND delegated QoS/threaded execution still works

### Requirement 3

The default full regression entrypoint SHALL include vendored worker regression cases.

#### Scenario: `non-qos`

- WHEN the user runs `./scripts/run_all_tests.sh non-qos`
- THEN the selected groups include a vendored worker regression step
- AND that step runs through the worker's own test entrypoint
- AND it is limited to a stable default subset instead of the full worker suite

### Requirement 4

Heavy threaded integration and vendored worker builds SHALL use constrained parallelism.

#### Scenario: building `mediasoup_thread_integration_tests`

- WHEN the scripts build `mediasoup_thread_integration_tests`
- THEN they invoke `cmake --build` with `-j1`
- AND other targets may continue using the normal configured parallelism

## Regression Expectations

- Required automated verification:
  - `bash -n scripts/run_all_tests.sh`
  - inspect target resolution for `non-qos` and default group sets
- Required manual smoke checks:
  - confirm the new build list excludes QoS/threaded artifacts for `non-qos`
