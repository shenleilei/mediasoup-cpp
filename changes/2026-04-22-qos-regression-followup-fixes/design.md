# Bugfix Design

## Context

This change is a mixed QoS regression follow-up, but the failures do not share one root cause.

The work should be split into 3 repair tracks:

1. downlink integration test contract correction
2. cpp harness timing hardening
3. matrix behavior regression repair

The important constraint is to keep these tracks logically separate:

- test contract fixes must not silently redefine runtime behavior
- harness timing fixes must not weaken assertions into non-signals
- matrix repairs must target real behavior, not expectation inflation

## Track 1: Downlink Seq-Reset Test Correction

### Diagnosis

`QosIntegrationTest.DownlinkClientStatsAcceptsSeqResetFromHighValue` currently combines mutually incompatible expectations:

- second request should be rejected as `stale-ts`
- yet `getStats` should later report `seq == 1`

The current downlink registry logic allows:

- stale lower seq within the reset threshold to be rejected as `stale-seq`
- large backward seq reset to be accepted
- regressed `tsMs` to be rejected as `stale-ts`

The test therefore appears outdated rather than exposing a runtime bug.

### Chosen Approach

- keep runtime registry logic unchanged unless targeted evidence disproves the current reading
- rewrite the failing gtest so it validates one coherent contract:
  - seq reset acceptance when `tsMs` remains forward
  - `stale-ts` rejection only when `tsMs` regresses
- verify neighboring stale-seq / stale-ts downlink cases together

### Why

- this is the narrowest change that restores consistency
- it aligns downlink coverage with the already accepted publisher-side seq-reset semantics
- it avoids destabilizing `DownlinkQosRegistry` without evidence

## Track 2: Cpp Harness Timing Hardening

### Diagnosis

The two `cpp-client-harness` failures are timing-sensitive:

- `multi_video_budget` reads one final sample after a fixed sleep and expects strict ordering on that single sample
- `threaded_multi_track_snapshot` stops waiting as soon as `clientStats.tracks` becomes an array, even if the array is still incomplete

Observed local repros show that:

- `multi_video_budget` can pass when rerun alone
- threaded multi-track snapshots can first appear as partial, then converge to the full track set

### Chosen Approach

- change `multi_video_budget` to use a short tail window and require ordering to be observed within that window
- change `threaded_multi_track_snapshot` to wait for the full expected track set, not merely any array
- keep current assertion strictness about actual track ids and ordering once the stable window is available

### Why

- this removes spurious failures without weakening the signal
- it mirrors the more robust waiting pattern already used in other threaded harness checks
- it preserves genuine regressions:
  - missing tracks still fail
  - no ordering within the observation window still fails

## Track 3: Matrix Behavior Repair

### Diagnosis

`BW3`, `T9`, and `S4` are not obviously harness-only failures.

Current reports show:

- `BW3`: insufficient degradation under 1000 kbps (`early_warning/L1` instead of `congested`)
- `T9`: impairment is correct, recovery never restarts within the allowed window
- `S4`: burst-jitter response overshoots to `congested/L4`

Existing project docs also show that at least one earlier targeted regression run passed `T9` and `S4`, so the present state is a behavior regression relative to accepted knowledge.

### Investigation Order

1. reproduce `BW3`, `T9`, `S4` in isolation with current runner and capture fresh diagnostics
2. compare with prior accepted docs/artifacts for:
   - runner topology
   - recovery fast-path assumptions
   - transition thresholds and timing
3. decide whether the failures share one cause

Likely branches:

- if `S4` and `BW3` both move with runner/network-shaping behavior, fix runner/network topology once
- if `T9` remains isolated, fix recovery behavior separately
- if targeted reruns already show flakiness, first harden the runner diagnostics before changing policy

### Chosen Guardrail

Do not broaden `sweep_cases.json` expectations for `BW3`, `T9`, or `S4` in the first pass.

Expectation changes are allowed only if:

- new evidence shows the existing accepted docs are outdated
- and the spec/docs are updated in the same change with explicit rationale

### Why

- `S4` already has prior history where a runner topology change created a fake behavior regression
- `T9` has dedicated recovery documentation and prior targeted pass evidence
- broadening the cases before isolating root cause would destroy the diagnostic value of the matrix

## Verification Strategy

### Track 1

- `./build/mediasoup_qos_integration_tests --gtest_filter=QosIntegrationTest.DownlinkClientStatsAcceptsSeqResetFromHighValue:QosIntegrationTest.DownlinkClientStatsRejectsRegressedTs:QosIntegrationTest.DownlinkClientStatsRejectsStaleSeq`

### Track 2

- `node tests/qos_harness/run_cpp_client_harness.mjs multi_video_budget`
- `node tests/qos_harness/run_cpp_client_harness.mjs threaded_multi_track_snapshot`
- `node tests/qos_harness/run_cpp_client_harness.mjs threaded_quick`

### Track 3

- `node tests/qos_harness/run_matrix.mjs --cases=BW3,T9,S4`
- if needed, add adjacent validation based on the fix direction:
  - `node tests/qos_harness/run_matrix.mjs --cases=R1,S4`
  - documented targeted combos around `T9`

### Delivery Gate

- `git diff --check`
- `docs/aicoding/DELIVERY_CHECKLIST.md` review before handoff

## Risks

- changing the downlink gtest could mask a real runtime bug if the current registry behavior is misread
- widening cpp harness wait windows too much could hide stalls instead of detecting them
- matrix fixes may affect nearby cases if they touch shared runner topology or recovery logic

## Mitigation

- verify neighboring downlink cases together, not in isolation
- keep harness waits bounded and still assert exact final invariants
- run targeted adjacent matrix cases after any matrix-facing fix
