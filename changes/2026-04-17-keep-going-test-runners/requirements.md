# Requirements

## Summary

Update the non-QoS and QoS test entrypoint behavior so a failing test task does not stop the overall script before the remaining selected tasks have run.

## Business Goal

Developers need a single run to surface the full failure set instead of rerunning the scripts repeatedly after each first failure.

## In Scope

- `scripts/run_all_tests.sh` failure handling
- confirmation and documentation of `scripts/run_qos_tests.sh` keep-going behavior
- summary output for selected test groups

## Out Of Scope

- internal parallel execution of test groups or tasks
- changing individual test binaries
- changing build targets or test coverage

## User Stories

### Story 1

As a developer
I want `scripts/run_all_tests.sh` to continue after one group fails
So that I can see all failing groups in one run

### Story 2

As a developer
I want the test scripts to end with a clear aggregated failure summary
So that I know what failed without scanning partial output only

## Acceptance Criteria

### Requirement 1

The system SHALL continue running remaining selected `run_all_tests.sh` groups after one group fails.

#### Scenario: Integration failure does not stop later groups

- WHEN one selected `run_all_tests.sh` group returns a non-zero exit status
- THEN the script SHALL execute the remaining selected groups
- AND the script SHALL exit non-zero after all selected groups complete

#### Scenario: Group-internal failure does not stop later tasks in that group

- WHEN one task inside the `integration` or `topology` group fails
- THEN later tasks in the same group SHALL still run
- AND the group SHALL be reported as failed in the final summary

### Requirement 2

The system SHALL print a final summary of failed groups for `run_all_tests.sh`.

#### Scenario: Multiple group failures

- WHEN more than one selected group fails
- THEN the final output SHALL list all failed groups together

### Requirement 3

The system SHALL preserve the existing keep-going behavior of `run_qos_tests.sh`.

#### Scenario: QoS task failure

- WHEN one QoS task fails inside `run_qos_tests.sh`
- THEN the script SHALL continue to later selected groups or tasks
- AND the final summary SHALL remain non-zero if any selected QoS group failed

## Non-Functional Requirements

- Performance: no material slowdown from the new failure aggregation logic
- Reliability: failure summaries must be deterministic and not depend on shell `set -e` side effects
- Security: no change
- Compatibility: existing CLI arguments remain valid
- Observability: final output must show enough context to identify failed groups

## Edge Cases

- A preflight failure such as a missing binary should still fail the affected group clearly
- A group with multiple failing subtasks should still only be listed once in the final group summary

## Open Questions

- None for this scoped change; internal parallelism is deferred follow-up work
