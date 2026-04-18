# Requirements

## Summary

Redefine `scripts/run_all_tests.sh` as the full C++ regression entrypoint and remove manual per-case QoS script filtering so every C++ gtest case in the repository is covered through stable binary-level entrypoints.

## Business Goal

Developers need one reliable native test entrypoint that covers the full C++ regression surface and leaves behind an up-to-date result document, without requiring manual stitching across `run_all_tests.sh`, `run_qos_tests.sh`, or raw logs.

## In Scope

- `scripts/run_all_tests.sh` group definitions, build targets, and execution behavior
- `scripts/run_qos_tests.sh` C++ group behavior
- CMake test target layout for QoS unit tests
- full C++ gtest coverage across the repository
- generated Markdown output for the latest full C++ regression run
- accepted spec and workflow docs for the updated entrypoint
- separating downlink QoS summary output from downlink detailed case output

## Out Of Scope

- moving browser, Node, client JS, or matrix QoS harnesses into `scripts/run_all_tests.sh`
- changing individual gtest implementations or their assertions
- adding archival history for full-run reports

## User Stories

### Story 1

As a developer
I want `scripts/run_all_tests.sh` to execute all C++ regression binaries, including QoS binaries
So that one command gives me the full native regression result.

### Story 2

As a developer
I want the generated report for `scripts/run_all_tests.sh` to reflect the new full-coverage scope
So that the latest result page matches what the script actually executed.

### Story 3

As a developer
I want filtered-out or mixed-location tests to stop falling through the gap between the two scripts
So that suites living in QoS binaries but validating broader behavior are still covered by the full regression entrypoint.

## Acceptance Criteria

### Requirement 1

The system SHALL make `scripts/run_all_tests.sh` the full C++ gtest regression entrypoint.

#### Scenario: default full run

- WHEN `scripts/run_all_tests.sh` runs with default groups
- THEN it SHALL execute every C++ gtest binary defined in `CMakeLists.txt`
- AND it SHALL include the QoS-specific binaries
- AND it SHALL cover the QoS-related unit and integration suites through the dedicated QoS binaries rather than script-level whitelist filters

#### Scenario: partial run

- WHEN only a subset of groups is selected
- THEN the script SHALL execute only the requested groups
- AND the selected groups SHALL still map to the updated full-C++ grouping model

### Requirement 2

The system SHALL close the current coverage holes created by script-level filtering and binary selection.

#### Scenario: mixed-scope tests inside QoS binary

- WHEN a test lives in `mediasoup_qos_integration_tests`
- THEN `scripts/run_all_tests.sh` SHALL cover it during a full run even if the case is not part of the narrower `run_qos_tests.sh` focused filter

#### Scenario: previously filtered unit suites

- WHEN a test suite lives in `mediasoup_tests`
- THEN `scripts/run_all_tests.sh` SHALL not exclude it only because its name matches a QoS-oriented filter pattern

#### Scenario: QoS regression script C++ groups

- WHEN `scripts/run_qos_tests.sh` runs `cpp-unit` or `cpp-integration`
- THEN it SHALL execute the dedicated QoS binaries directly
- AND it SHALL not depend on manually maintained per-case whitelist filters

### Requirement 3

The system SHALL rewrite a result document whose name and content match the updated entrypoint semantics.

#### Scenario: actual execution

- WHEN `scripts/run_all_tests.sh` finishes after running one or more groups
- THEN it SHALL rewrite a Markdown result page for the latest full C++ regression run
- AND the page SHALL include generation time, selected groups, overall status, failed-task summary, and per-task duration details

#### Scenario: help or list

- WHEN `scripts/run_all_tests.sh --help` or `--list` exits before running tests
- THEN the generated result page SHALL NOT be rewritten by that invocation

### Requirement 4

The system SHALL keep downlink QoS summary output separate from downlink detailed case output.

#### Scenario: regular QoS script execution

- WHEN `scripts/run_qos_tests.sh` finishes any actual run
- THEN it SHALL rewrite a dedicated downlink summary page
- AND it SHALL not overwrite the full downlink case report file with that summary

#### Scenario: downlink matrix render

- WHEN `downlink-matrix` runs and the render input exists
- THEN the detailed case report SHALL still be written to the downlink case-report path
- AND that detailed report SHALL remain distinct from the summary page

## Non-Functional Requirements

- Performance: the script should avoid unnecessary duplicate work beyond what existing binaries already define
- Reliability: missing binaries or preflight dependencies must remain visible as failed tasks in the report
- Compatibility: existing continue-on-failure behavior and final non-zero exit on failure must remain intact
- Clarity: docs and generated output must no longer describe the entrypoint as non-QoS-only

## Edge Cases

- QoS unit coverage currently lives in mixed source files and dedicated QoS binaries; the implementation must keep ownership clear enough that new cases do not require script-level case whitelists
- Redis-backed groups must still surface missing `redis-server` as preflight failures
- Partial runs must not rewrite a report that claims unselected groups were attempted

## Open Questions

- None; browser/client-JS/matrix QoS harnesses remain intentionally out of this change because they materially change the runtime environment contract of `scripts/run_all_tests.sh`
