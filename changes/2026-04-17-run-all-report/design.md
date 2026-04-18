# Design

## Context

`scripts/run_all_tests.sh` currently builds binaries, runs the selected non-QoS groups, and prints PASS/FAIL lines to stdout. Unlike `scripts/run_qos_tests.sh`, it does not leave behind a Markdown result artifact under `docs/`.

## Proposed Approach

Extend `scripts/run_all_tests.sh` with lightweight result tracking and Markdown rendering:

- add ordered task labels for each attempted command
- have `run_cmd()` measure elapsed time and record status and duration
- record preflight failures that happen before a binary starts as failed tasks with a placeholder duration
- generate `docs/non-qos-test-results.md` after the selected groups finish, regardless of overall pass/fail result

The report will contain:

- generation time
- script path and selected groups
- overall status
- failed-group and failed-task summary
- per-task details for every attempted binary or preflight check

## Alternatives Considered

- Alternative: generate a JSON artifact first, then render Markdown
- Why it was rejected: the non-QoS entrypoint does not currently need archival or downstream processing; Markdown-only is the smallest change that satisfies the user request

- Alternative: update an existing README section in place
- Why it was rejected: README is stable workflow documentation, not a run artifact

## Modules And Responsibilities

- `scripts/run_all_tests.sh`: collect task results and render the Markdown report
- `docs/non-qos-test-results.md`: latest generated non-QoS run artifact
- `README.md`, `docs/DEVELOPMENT.md`, `docs/README.md`: point readers to the generated report
- `specs/current/test-entrypoints.md`: capture accepted script behavior

## Data And State

- `FAILED_GROUPS`: ordered list of failed top-level groups
- `TASK_ORDER`: ordered list of attempted tasks or preflight checks
- `TASK_RESULTS[label]`: `PASS` or `FAIL`
- `TASK_DURATIONS[label]`: elapsed time or `-` for preflight failures
- `SELECTED_GROUPS`: the effective selected groups for the run

## Interfaces

- no new CLI arguments
- no changed build targets
- new generated artifact: `docs/non-qos-test-results.md`

## Failure Handling

- wrapped task commands keep their current PASS/FAIL terminal output and continue behavior
- preflight failures use explicit task labels like `integration:preflight:redis-server`
- the report is written before the script exits with the final aggregated status
- `--help` and `--list` still exit early without report generation

## Testing Strategy

- syntax-check `scripts/run_all_tests.sh`
- run `./scripts/run_all_tests.sh --skip-build unit` and verify the report content
- run `./scripts/run_all_tests.sh --list` before and after a test run to confirm listing does not rewrite the report

## Observability

- the new Markdown report complements, not replaces, terminal PASS/FAIL lines
- failed groups and failed tasks appear in a stable summary section for easier review

## Rollout Notes

- backward compatible; readers who do not need the artifact can ignore it
- rollback is limited to the new report file, script helpers, and documentation references
