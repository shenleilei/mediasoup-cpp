# Requirements

## Summary

Redefine `scripts/run_all_tests.sh` as the one-command full repository regression entrypoint by making it cover the complete QoS regression surface currently owned by `scripts/run_qos_tests.sh`, not just the native C++ QoS binaries.

## Business Goal

Developers need one command that exercises the repository's accepted regression surface without having to remember that "full" still excludes the JS, harness, browser, matrix, and threaded QoS paths.

## In Scope

- `scripts/run_all_tests.sh` group behavior, build stage, status text, and generated report
- delegation from `scripts/run_all_tests.sh` to `scripts/run_qos_tests.sh`
- report naming and docs/spec updates for the new entrypoint semantics
- preserving a targeted threaded-only entry for users who still want just that slice

## Out Of Scope

- changing individual QoS test implementations or assertions
- flattening every `run_qos_tests.sh` subtask into `run_all_tests.sh` as a separately managed local implementation
- changing `scripts/run_qos_tests.sh`'s own group model or output artifacts beyond what `run_all_tests.sh` depends on

## User Stories

### Story 1

As a developer
I want `scripts/run_all_tests.sh` to cover all accepted QoS regression paths
So that one command actually means "full regression" for this repository.

### Story 2

As a developer
I want `scripts/run_all_tests.sh` to keep using the existing QoS regression script rather than reimplementing it
So that the QoS-only entrypoint remains the source of truth for QoS-specific flow control, reports, and failure handling.

### Story 3

As a developer
I want the generated report and docs to stop claiming this is only a full C++ run
So that the entrypoint contract matches reality.

## Acceptance Criteria

### Requirement 1

The system SHALL make `scripts/run_all_tests.sh` the full repository regression entrypoint.

#### Scenario: default full run

- WHEN `scripts/run_all_tests.sh` runs with its default group selection
- THEN it SHALL execute the existing non-QoS native groups
- AND it SHALL also execute the full QoS regression coverage defined by `scripts/run_qos_tests.sh`
- AND the default run SHALL avoid double-running the threaded QoS slice

#### Scenario: qos-only run

- WHEN `scripts/run_all_tests.sh qos` runs
- THEN it SHALL execute the full QoS regression entrypoint
- AND that SHALL include the threaded QoS slice owned by `scripts/run_qos_tests.sh`

#### Scenario: threaded-only compatibility run

- WHEN `scripts/run_all_tests.sh threaded` runs without `qos`
- THEN it SHALL execute only the threaded QoS regression slice
- AND this compatibility path SHALL remain available for focused reruns

### Requirement 2

The system SHALL keep `scripts/run_qos_tests.sh` as the QoS-specific source of truth.

#### Scenario: qos delegation

- WHEN `scripts/run_all_tests.sh` runs a QoS-bearing group
- THEN it SHALL delegate to `scripts/run_qos_tests.sh`
- AND it SHALL rely on that script to manage QoS-specific reports and failure semantics

#### Scenario: build prerequisites

- WHEN `scripts/run_all_tests.sh` performs its build stage before QoS delegation
- THEN it SHALL build the repo-owned binaries needed by the delegated QoS regression flow
- AND missing external dependencies outside the repo build graph SHALL still surface as test failures, not silent skips

### Requirement 3

The system SHALL rename the generated report and documentation wording to match the new entrypoint semantics.

#### Scenario: actual execution

- WHEN `scripts/run_all_tests.sh` finishes after running one or more groups
- THEN it SHALL rewrite a Markdown result page for the latest full regression run
- AND that page SHALL no longer be named or described as "full C++ only"

#### Scenario: help or list

- WHEN `scripts/run_all_tests.sh --help` or `--list` exits before running tests
- THEN the generated full-regression report SHALL NOT be rewritten

## Non-Functional Requirements

- Reliability: continue-on-failure behavior and final non-zero exit semantics must remain intact
- Clarity: CLI help, specs, and workflow docs must describe `run_all_tests.sh` as the full regression entrypoint
- Maintainability: QoS logic should stay centralized in `scripts/run_qos_tests.sh` rather than being duplicated into `scripts/run_all_tests.sh`

## Edge Cases

- Default group selection currently includes a separate `threaded` group; the new behavior must prevent duplicate execution when `qos` already covers threaded QoS
- `scripts/run_all_tests.sh` should keep working for focused reruns of `threaded` even after `qos` becomes the full QoS umbrella
- The generated report path rename must not leave docs pointing at the old file name

## Open Questions

- None
