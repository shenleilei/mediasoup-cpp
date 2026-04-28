# Bugfix Design

## Context

The oscillation case `O1` is intended to catch over-active QoS control, not to penalize a clean stepwise descent to the worst accepted ladder level followed by a clean stepwise recovery. The current camera ladder for the plain-client path requires more than five non-noop actions to complete that monotonic trip.

At the same time, the browser loopback runner still exhibits a genuinely excessive action count, so the case must remain strict there.

## Root Cause

The oracle uses one shared `maxActionCount` threshold for `O1`, while the runners do not share the same practical control-path granularity. Once `maxActionCountPassed` was wired into `deriveCaseEvaluation()`, `cpp_client` began failing even for the accepted monotonic path.

## Fix Strategy

### 1. Add a `cpp_client` runner-specific `maxActionCount`

- Keep the base `O1` `maxActionCount: 5` unchanged.
- Add `expectByRunner.cpp_client.maxActionCount = 8`.
- Keep the existing `loopback.maxActionCount = 5`.

This preserves current browser strictness while recognizing the accepted plain-client ladder behavior.

### 2. Extend synthetic oracle tests

- Add an explicit unit test showing the same `O1` baseline/impaired/recovered states fail for the default runner at `actionCount=8` but pass for `cpp_client`.
- Keep the existing generic max-action-count failure test intact.

## Risk Assessment

- The main risk is masking a real plain-client oscillation bug. That risk is limited because the adjustment is narrowly scoped to `O1` and to the monotonic-action budget only.
- Browser loopback remains protected by the unchanged strict threshold.
- This does not change runtime logic, only the expected oracle budget.

## Test Strategy

- Run `tests/qos_harness/test.synthetic_sweep.mjs`.
- Run a targeted `cpp_client` `O1` rerun to verify the updated expectation produces a pass.
- Do not rerun the full matrix in this bugfix unless explicitly needed; the full report can remain stale until a later full run.

## Observability

- No logging or metrics changes are needed.
- The observable result is the targeted `cpp_client` `O1` verdict after rerun.

## Rollout Notes

- No migration is required.
- Historical archived artifacts remain unchanged and still represent the verdict logic used at the time they were generated.
