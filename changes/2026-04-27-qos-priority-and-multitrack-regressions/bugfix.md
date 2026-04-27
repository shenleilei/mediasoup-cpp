# Bugfix Plan: QoS Priority And Multi-track Regressions

## Symptom
Two QoS regression paths remain broken after the recent browser/plain-client follow-up work:

1. `browser-harness:downlink-priority` fails `NoRegressionWithoutThrottle`, pausing visible consumers even without active netem throttle.
2. `cpp-client-harness:multi_video_budget` and `threaded_multi_video_budget` fail to preserve the intended weighted ordering for multi-track video under constrained transport budget.

## Reproduction
1. Run `scripts/run_qos_tests.sh browser-harness:downlink-priority`.
2. Observe `NoRegressionWithoutThrottle` failing with both consumers paused and transport bitrate reported as `0kbps`.
3. Run `scripts/run_qos_tests.sh cpp-client-harness:multi_video_budget`.
4. Observe the highest-weight track failing to stay above the lower-weight tracks.
5. Run `node tests/qos_harness/run_cpp_client_harness.mjs threaded_multi_video_budget`.
6. Observe server-side `clientStats.tracks[].signals.sendBitrateBps` reporting only the first video track as active while later tracks remain at zero.

## Observed Behavior
- The downlink allocator treats early `availableIncomingBitrate=0` samples as hard zero budget and pauses visible consumers before startup has stabilized.
- The plain-client multi-track budget scenario does not produce a reliable per-track weighted ordering:
  - legacy/local trace assertions can invert the expected order
  - threaded/server-side client stats can report only the first track as active

## Expected Behavior
- A visible browser subscriber with no active artificial throttle SHALL NOT be auto-paused merely because startup transport stats have not produced a positive incoming bitrate yet.
- The QoS multi-track budget path SHALL produce internally consistent per-track budget caps and observable per-track send statistics for all published video tracks.
- Weighted multi-track regression coverage SHALL validate the actual supported behavior using synchronized and trustworthy observations instead of a misleading local-only proxy.

## Suspected Scope
- `src/qos/SubscriberBudgetAllocator.*`
- Downlink QoS unit/integration coverage
- `client/PlainClientThreaded.cpp`
- Possibly supporting plain-client stats or transport accounting modules under `client/`
- `tests/qos_harness/run_cpp_client_harness.mjs`
- Related multi-track runtime and regression tests

## Known Non-affected Behavior
- Netem guard / matrix contamination cleanup is already fixed in the current branch baseline.
- Browser demo interop and recv-transport RTP capability fixes are separate issues and not the primary root cause for these two regressions.

## Acceptance Criteria
- `scripts/run_qos_tests.sh browser-harness:downlink-priority` passes, including `NoRegressionWithoutThrottle`.
- Downlink allocator coverage proves that startup `availableIncomingBitrate=0` does not force visible consumers into an immediate zero-budget pause path when the system has not yet observed real congestion.
- `cpp-client-harness:multi_video_budget` passes using a trustworthy observation path.
- `threaded_multi_video_budget` passes and proves that all published video tracks appear in server-side `clientStats` with non-broken `sendBitrateBps` accounting and weighted ordering expectations.
- Any updated expectations are reflected in accepted specs if the supported runtime contract changes.

## Regression Expectations
- Existing explicit congestion cases that legitimately rely on tight budget behavior remain covered.
- Multi-track QoS stats stay consistent across local traces, threaded runtime state, and server-side `clientStats`.
