# Bugfix Analysis

## Summary

The `O1` oscillation oracle currently fails the PlainTransport C++ client (`cpp_client`) path on action-count budget alone, even when the path follows the expected monotonic degrade-to-`L4` and monotonic recovery-to-`L0` sequence without repeated same-level thrash.

The browser loopback path remains genuinely over-active and should stay strict.

## Reproduction

1. Inspect `tests/qos_harness/scenarios/sweep_cases.json` for case `O1`.
2. Compare the current `cpp_client` `O1` result in `docs/generated/uplink-qos-cpp-client-matrix-report.json`.
3. Observe `stateMatch=true`, `levelMatch=true`, `recoveryPassed=true`, but `maxActionCountPassed=false` with `actionCount=8`.
4. Compare the browser loopback `O1` result in `docs/generated/uplink-qos-matrix-report.json` and observe much larger action counts.

## Observed Behavior

- `O1` uses a shared `maxActionCount: 5` budget for all runners.
- The `cpp_client` path legitimately reaches `L4` and returns to `L0`, which currently costs 8 non-noop actions because the accepted camera ladder is stepwise.
- The browser loopback path still produces far more actions and remains a real oscillation problem.

## Expected Behavior

- The `cpp_client` runner SHALL be allowed the minimal full ladder descent-and-recovery action budget required by the accepted camera ladder.
- The loopback/browser runner SHALL keep the stricter `O1` action-count guard.

## Known Scope

- `tests/qos_harness/scenarios/sweep_cases.json`
- `tests/qos_harness/test.synthetic_sweep.mjs`
- targeted `cpp_client` QoS matrix verification

## Must Not Regress

- Browser/loopback `O1` should still fail when it remains over-active.
- Other sweep-case runner overrides must continue merging the same way.
- No runtime QoS logic should change in this bugfix; only the oracle expectation changes.

## Suspected Root Cause

`O1` inherited a runner-agnostic `maxActionCount: 5` budget even though the `cpp_client` implementation uses the accepted stepwise camera ladder. The oracle therefore started failing `cpp_client` as soon as action-count checks were wired into verdict calculation, despite the path not exhibiting the repeated-action pattern the case is meant to catch.

## Acceptance Criteria

### Requirement 1

The system SHALL apply a runner-specific `O1` action-count allowance for `cpp_client`.

#### Scenario: PlainTransport C++ client `O1`

- WHEN `deriveCaseEvaluation()` evaluates case `O1` for runner `cpp_client`
- THEN the runner-specific action-count expectation allows the monotonic `L0 -> L4 -> L0` path
- AND the same trace no longer fails solely because `actionCount=8`

### Requirement 2

The loopback/browser `O1` oracle SHALL remain strict.

#### Scenario: Loopback `O1`

- WHEN the loopback runner exceeds the strict `O1` action-count budget
- THEN the verdict still fails

## Regression Expectations

- Existing unaffected behavior: other sweep-case expectations, browser/downlink renderers, stored verdict handling.
- Required automated regression coverage: synthetic sweep oracle unit tests for runner-specific `O1`.
- Required manual smoke checks: targeted `cpp_client` `O1` rerun after the oracle change.
