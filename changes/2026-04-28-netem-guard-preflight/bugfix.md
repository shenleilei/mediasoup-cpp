# Bugfix Analysis

## Summary

Targeted reruns can still fail before executing any QoS logic because a previous run leaves a `netem guard` lock behind. The current behavior only discovers that after waiting for the full guard timeout.

## Reproduction

1. Leave a live or stale `netem guard` lock under `/tmp/mediasoup-qos-netem-locks`.
2. Start a new loopback QoS task such as `run_cpp_client_matrix.mjs`.
3. Observe the task waiting until `acquireNetemGuard()` times out.

## Observed Behavior

- stale guards are only reclaimed opportunistically during acquisition
- live conflicting guards are reported late, after the caller already spent the full wait timeout
- `run_qos_tests.sh` does not proactively inspect loopback netem guard state before launching a new netem-dependent task

## Expected Behavior

- the harness SHALL sweep stale netem guards before each netem-dependent run
- a conflicting live guard SHALL be surfaced immediately with owner details
- operators MAY opt into force-clearing live guards explicitly, but the default behavior SHALL remain safe

## Known Scope

- `tests/qos_harness/netem_guard.mjs`
- `tests/qos_harness/test.netem_guard.mjs`
- `tests/qos_harness/preflight_netem_guards.mjs`
- `scripts/run_qos_tests.sh`

## Acceptance Criteria

### Requirement 1

Loopback netem tasks SHALL preflight guard state before launch.

#### Scenario: stale guard exists

- WHEN a stale loopback guard exists before a netem-dependent run
- THEN preflight removes it immediately instead of waiting for acquisition timeout

### Requirement 2

Live guard conflicts SHALL fail fast by default.

#### Scenario: another process still owns the loopback guard

- WHEN a netem-dependent run starts while a live loopback guard exists
- THEN preflight exits with an explicit owner report
- AND the task does not wait for the full acquisition timeout

### Requirement 3

Force-clear SHALL remain explicit.

#### Scenario: operator opts into forced guard cleanup

- WHEN `QOS_FORCE_CLEAR_NETEM_GUARDS=1` is set
- THEN preflight may clear even live guards and reset the affected qdisc before the run starts
