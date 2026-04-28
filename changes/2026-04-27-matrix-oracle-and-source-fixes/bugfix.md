# Bugfix Plan: Matrix Oracle And Source Fixes

## Symptom
The QoS matrix regression still misclassifies some targeted cases even when the runtime behavior is acceptable:

- `O1` fails because runner-specific expectations replace the base expectation instead of extending it, and `maxActionCount` is not evaluated correctly.
- `M1` traffic-model loopback cases do not actually exercise the declared source semantics (`screenshare` / `audio`) in the browser loopback harness.

## Reproduction
1. Run `scripts/run_qos_tests.sh --matrix-cases=O1 matrix`.
2. Observe `stateMatch=true`, `levelMatch=true`, `recoveryPassed=true`, but the case still fails with `analysis=过强`.
3. Run `scripts/run_qos_tests.sh --matrix-cases=M1 matrix`.
4. Observe the loopback harness behaving like a camera-source run rather than a screen-share traffic model, or otherwise failing to align with the intended source model.

## Observed Behavior
- `expectByRunner.loopback` drops base expectation fields such as `states` and `maxLevel`, leaving only `maxActionCount`.
- Matrix action counting treats repeated identical `setEncodingParameters` samples as separate oscillation actions.
- Browser loopback harness always builds a camera-style sender/controller regardless of case source.

## Expected Behavior
- Runner-specific expectations SHALL merge with base expectations instead of replacing them wholesale.
- `maxActionCount` SHALL participate in verdict logic using a meaningful action count rather than raw repeated identical samples.
- Loopback matrix harness SHALL honor declared `source` semantics for traffic-model cases, especially `screenShare`.

## Suspected Scope
- `tests/qos_harness/synthetic_sweep_shared.mjs`
- `tests/qos_harness/run_matrix.mjs`
- `tests/qos_harness/run_cpp_client_matrix.mjs`
- `tests/qos_harness/browser/loopback-entry.js`
- `tests/qos_harness/loopback_runner.mjs`
- `tests/qos_harness/test.synthetic_sweep.mjs`

## Acceptance Criteria
- `scripts/run_qos_tests.sh --matrix-cases=O1 matrix` passes for the right reason.
- `scripts/run_qos_tests.sh --matrix-cases=M1 matrix` runs with the intended source model and passes.
- Shared synthetic-sweep tests cover:
  - merged runner expectations
  - `maxActionCount` verdict participation
- Matrix reports use meaningful action-count semantics.

## Regression Expectations
- Existing widened runner-specific expectations such as `T1` remain valid.
- Matrix/cpp-client report generation continues to work with the updated action-count semantics.
