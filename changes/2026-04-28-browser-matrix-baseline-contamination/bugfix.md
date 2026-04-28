# Bugfix Analysis

## Summary

The browser loopback matrix currently treats some legitimate weak-baseline cases as infrastructure failures before the actual verdict logic runs.

The concrete failure observed in `/var/log/run_all_tests.log` is case `B3`, which is a baseline-group case that is expected to degrade to `early_warning` or `congested`, but is instead aborted as `baseline contamination`.

## Reproduction

1. Run `tests/qos_harness/run_matrix.mjs`.
2. Observe case `B3` aborting with:
   `baseline contamination detected for B3: baseline entered recovering before any impairment`
3. Compare this with `tests/qos_harness/scenarios/sweep_cases.json`, where `B3` explicitly expects degraded baseline behavior.

## Observed Behavior

- `run_matrix.mjs` applies `detectBaselineContamination()` uniformly to all cases.
- `baseline` group cases are aborted before verdict evaluation if their baseline state looks degraded.

## Expected Behavior

- `baseline` group cases SHALL be evaluated by their declared oracle expectations, not by the baseline-contamination infrastructure heuristic.
- The contamination heuristic MAY still protect non-baseline cases from leaked netem or pre-impairment corruption.

## Known Scope

- `tests/qos_harness/run_matrix.mjs`
- helper/test files for browser matrix baseline contamination logic

## Acceptance Criteria

### Requirement 1

Baseline-group cases SHALL bypass the contamination heuristic.

#### Scenario: B3 weak baseline

- WHEN `detectBaselineContamination()` is evaluated for case `B3`
- THEN it returns `null`
- AND the case continues to normal verdict evaluation

### Requirement 2

Non-baseline mild-network cases SHALL keep the existing contamination protection.

#### Scenario: mild baseline on a non-baseline case

- WHEN a non-baseline case enters `recovering` before impairment
- THEN the contamination helper still reports an infrastructure failure
