# Design

## Context

`scripts/run_all_tests.sh` currently runs groups sequentially under `set -e`, so the first non-zero result aborts the script. `scripts/run_qos_tests.sh` already wraps most task commands in a `run_cmd()` helper that captures non-zero exit codes and records them without aborting the entire run.

## Proposed Approach

Bring `scripts/run_all_tests.sh` in line with the QoS runner model:

- introduce a `FAILED_GROUPS` accumulator
- make `run_cmd()` capture and return command status rather than relying on shell fail-fast behavior
- make `run_integration()` and `run_topology()` accumulate per-task failures instead of aborting on the first failed binary
- make the top-level group loop continue across failed groups and emit an aggregated final summary

`scripts/run_qos_tests.sh` will not get behavior changes unless a gap is found; its current aggregated-failure flow is already the reference model for this scoped task.

## Alternatives Considered

- Alternative: remove `set -e` globally from both scripts
- Why it was rejected: too broad and makes preflight/setup failures easier to miss or mishandle

- Alternative: add a new `--keep-going` flag only
- Why it was rejected: the user asked for this behavior now, and `run_qos_tests.sh` already behaves that way by default

## Modules And Responsibilities

- `scripts/run_all_tests.sh`: add keep-going execution and aggregated summary
- `scripts/run_qos_tests.sh`: confirm behavior remains consistent
- docs: describe the run-all summary behavior accurately

## Data And State

- `FAILED_GROUPS`: ordered list of failed top-level groups for `run_all_tests.sh`
- local `failed` flags in grouped runners to preserve group-level non-zero status when any subtask fails

## Interfaces

- no new external APIs
- no removed CLI options
- output changes:
  - task-level PASS/FAIL lines for `run_all_tests.sh`
  - final aggregated group summary

## Failure Handling

- wrapped task command failures are recorded and execution continues
- missing required files or commands still fail the relevant group explicitly
- final process exit remains non-zero if any selected group failed

## Security Considerations

- none; shell behavior only

## Testing Strategy

- syntax check for both scripts
- run `run_all_tests.sh` with a deliberately failing selected group plus a later passing group to prove keep-going behavior
- run a targeted `run_qos_tests.sh` selection to confirm no regression in its existing summary behavior

## Observability

- retain explicit task labels and add a final failed-group summary for `run_all_tests.sh`

## Rollout Notes

- backward compatible for CLI usage
- if unexpected behavior appears, rollback is limited to `scripts/run_all_tests.sh` and documentation
