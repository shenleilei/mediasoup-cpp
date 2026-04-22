# QoS Regression Follow-Up Implementation Plan

## Change Context

- Type: Structured Change
- Change folder: `changes/2026-04-22-qos-regression-followup-fixes`
- Objective: clear the remaining full-regression QoS failures without mixing test-contract cleanup, harness timing hardening, and real matrix behavior repairs into one opaque patch

## Scope

### In Scope

- downlink seq-reset integration coverage
- cpp plain-client harness timing and convergence checks
- matrix regressions `BW3`, `T9`, `S4`
- required docs/spec updates that reflect the final accepted behavior

### Out Of Scope

- unrelated QoS test failures not present in the current full-regression log
- broad scenario-catalog expectation rewrites without root-cause evidence
- general refactors of the harness framework

## Constraints

- do not alter runtime semantics to satisfy a likely stale test without evidence
- do not weaken harness assertions into “eventually anything is fine”
- do not broaden matrix expectations until the regression root cause is isolated
- verification must stay targeted first, then expand only where the fix direction requires it

## Execution Order

1. Fix the clearly inconsistent downlink gtest contract first.
   Reason: highest-confidence issue, smallest scope, removes one red test without affecting runtime behavior.

2. Harden cpp harness timing next.
   Reason: also high-confidence, should reduce false reds before spending time on real behavior regressions.

3. Reproduce `BW3`, `T9`, `S4` again after the above two tracks are clean.
   Reason: this avoids debugging matrix behavior while the surrounding QoS signal is still noisy.

4. Implement the narrowest matrix-facing fix based on targeted evidence.

5. Update docs/specs only after the final behavior is confirmed.

## Verification Map

### Fast Checks

- `./build/mediasoup_qos_integration_tests --gtest_filter=QosIntegrationTest.DownlinkClientStatsAcceptsSeqResetFromHighValue:QosIntegrationTest.DownlinkClientStatsRejectsRegressedTs:QosIntegrationTest.DownlinkClientStatsRejectsStaleSeq`
- `node tests/qos_harness/run_cpp_client_harness.mjs multi_video_budget`
- `node tests/qos_harness/run_cpp_client_harness.mjs threaded_multi_track_snapshot`
- `node tests/qos_harness/run_cpp_client_harness.mjs threaded_quick`

### Matrix Checks

- `node tests/qos_harness/run_matrix.mjs --cases=BW3,T9,S4`

### Expansion Checks

- neighboring matrix cases selected according to the final fix direction
- any doc/spec-targeted verification required by changed accepted behavior

### Final Hygiene

- `git diff --check`
- `docs/aicoding/DELIVERY_CHECKLIST.md`

## Risks And Decision Points

- If the rebuilt evidence shows the downlink test is not stale but the registry really accepts a regressed timestamp, stop and treat Track 1 as a runtime bug.
- If cpp harness changes still fail in isolated runs after bounded wait hardening, stop treating them as timing-only and reclassify as runtime defects.
- If `BW3`, `T9`, and `S4` split across unrelated root causes, split implementation into separate patches but keep this change folder as the umbrella execution record.
