## Symptom

QoS browser and loopback matrix runs can leave `tc qdisc dev lo root` in a weak-network configuration after failure or interruption. Subsequent harnesses inherit the stale loopback netem state and produce cascaded false failures such as baseline cases starting in `recovering` or `congested`.

## Reproduction

1. Run `scripts/run_all_tests.sh --skip-build qos` or `scripts/run_qos_tests.sh all`.
2. Let a netem-based browser harness fail or interrupt the run while later matrix cases are still pending.
3. Inspect `tc qdisc show dev lo` and observe residual `netem` state.
4. Continue into `tests/qos_harness/run_matrix.mjs` and observe baseline cases such as `B1` begin from a degraded state.

## Observed Behavior

- `tc qdisc show dev lo` still reports a configured root `netem` after the previous harness stops.
- Browser loopback and matrix cases report impossible baseline states, for example `baseline(current=recovering/L3)` or `baseline(current=congested/L4)` for mild scenarios.
- One harness failure can cascade into many unrelated matrix failures.

## Expected Behavior

- Netem-based harnesses must hold exclusive ownership of `lo` root qdisc while running.
- Harness startup must clean stale root qdisc state left by prior runs.
- Harness shutdown must always clear root qdisc state on normal exit and common failure signals.
- Matrix-style runners must fail fast on infrastructure contamination instead of producing long chains of false case failures.

## Suspected Scope

- `tests/qos_harness/loopback_runner.mjs`
- `tests/qos_harness/cpp_client_runner.mjs`
- `tests/qos_harness/browser_loopback.mjs`
- `tests/qos_harness/browser_downlink_priority.mjs`
- `scripts/run_qos_tests.sh`

## Known Non-Affected Behavior

- Public interop browser harness does not modify `tc qdisc` and is not part of the contamination path.
- Pure gtest suites that do not alter loopback netem are unaffected by this bug.

## Acceptance Criteria

1. All netem-based QoS harnesses serialize access to `lo` root qdisc through a shared guard.
2. Harness startup clears stale `lo` root qdisc state before applying new netem settings.
3. Harness shutdown clears `lo` root qdisc state on normal exit and signal-driven exit.
4. `scripts/run_qos_tests.sh` clears loopback root qdisc before and after netem-based groups.
5. `run_matrix.mjs` aborts early with an infrastructure error if baseline state is clearly contaminated before meaningful impairment begins.

## Regression Expectations

- Targeted reruns of `browser-harness:downlink-priority` and a focused matrix slice (`B1,L1,L3,L4,R1,R2,R3,R4,J1,J2,J3`) must run from a clean `lo` qdisc state and avoid the prior cascade caused by stale netem state.
