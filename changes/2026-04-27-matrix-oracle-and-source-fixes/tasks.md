# Tasks

1. [x] Merge runner-specific expectations with base expectations.
   - Files: `tests/qos_harness/synthetic_sweep_shared.mjs`, tests
   - Outcome: runner overrides no longer discard base state/level bounds.
   - Verify: `node --test tests/qos_harness/test.synthetic_sweep.mjs`

2. [x] Count meaningful action changes for oscillation verdicts.
   - Files: `tests/qos_harness/synthetic_sweep_shared.mjs`, `run_matrix.mjs`, `run_cpp_client_matrix.mjs`
   - Outcome: `maxActionCount` gates oscillation meaningfully instead of counting repeated identical samples.
   - Verify: targeted `O1`

3. [x] Honor declared source semantics in the loopback matrix harness.
   - Files: `tests/qos_harness/browser/loopback-entry.js`, `tests/qos_harness/loopback_runner.mjs`, `tests/qos_harness/run_matrix.mjs`
   - Outcome: traffic-model cases such as `M1` actually run as screen-share in loopback mode.
   - Verify: targeted `M1`

4. [x] Re-run targeted matrix verification and keep docs truthful.
   - Files: affected docs/results if needed
   - Outcome: `M1` and `O1` pass with aligned oracle semantics.
   - Verify: targeted reruns
