# Requirements

## Summary

Add generated Markdown reporting to `scripts/run_all_tests.sh` so the latest non-QoS regression result is written to project docs after each actual test run.

## Business Goal

Developers need the non-QoS entrypoint to leave behind a shareable, up-to-date result page, instead of requiring readers to inspect raw terminal logs only.

## In Scope

- `scripts/run_all_tests.sh` result recording
- generated Markdown output for the latest non-QoS run
- workflow documentation for the new report artifact
- accepted spec for the non-QoS report contract

## Out Of Scope

- adding JSON artifacts or archive snapshots
- changing individual test binaries or coverage
- changing `scripts/run_qos_tests.sh`
- backfilling historical non-QoS runs

## User Stories

### Story 1

As a developer
I want `scripts/run_all_tests.sh` to refresh a Markdown report
So that I can review the latest non-QoS regression result without reopening the full log.

### Story 2

As a developer
I want the report to include selected groups and per-task outcomes
So that partial runs and failed subtasks are visible in a durable place.

## Acceptance Criteria

### Requirement 1

The system SHALL rewrite `docs/non-qos-test-results.md` after a real `scripts/run_all_tests.sh` execution completes.

#### Scenario: Full run succeeds

- WHEN `scripts/run_all_tests.sh` finishes after running selected test groups
- THEN `docs/non-qos-test-results.md` SHALL exist
- AND it SHALL include generation time, selected groups, and the final overall status

#### Scenario: Selected subset runs

- WHEN only a subset of groups is selected
- THEN the report SHALL record the selected groups for that run
- AND it SHALL only list the tasks that were attempted during that run

### Requirement 2

The system SHALL record per-task status for the non-QoS entrypoint.

#### Scenario: Group with multiple binaries

- WHEN `integration` or `topology` runs multiple binaries
- THEN the report SHALL show each attempted binary separately
- AND each entry SHALL include pass/fail status and elapsed time

#### Scenario: Failing task

- WHEN any attempted task fails
- THEN the report SHALL show the task as failed
- AND the report summary SHALL identify failed groups and failed tasks

### Requirement 3

The system SHALL not rewrite the report for non-execution flows.

#### Scenario: Help or list

- WHEN `scripts/run_all_tests.sh --help` or `--list` exits before running tests
- THEN `docs/non-qos-test-results.md` SHALL NOT be rewritten by that invocation

## Non-Functional Requirements

- Performance: report generation must add negligible overhead compared with test execution time
- Reliability: report content must be deterministic for a given run result
- Compatibility: existing CLI options and exit semantics must remain valid
- Observability: the report must make failures visible without needing the raw console log

## Edge Cases

- A missing binary or command should still produce a failed group and a failed task entry when that preflight check belongs to an attempted group
- A group with multiple failed subtasks should appear once in the failed-groups summary and multiple times in failed-task details
- `--skip-build` should still generate the report if tests are actually executed

## Open Questions

- None; this change intentionally uses a single generated Markdown result page rather than introducing a broader artifact pipeline
