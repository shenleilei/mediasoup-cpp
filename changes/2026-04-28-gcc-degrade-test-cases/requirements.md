# Requirements

## Summary

Add targeted GCC degradation test cases (`gcc_degrade` group) to the QoS
scenario catalog as **extended** cases that do not enter the default 43-case
main gate.

## Business Goal

The existing sweep cases test bandwidth, RTT, loss, and jitter individually,
but none specifically target the two real GCC degradation triggers:

1. **High packet loss (≥5%)** — activates GCC's loss-based controller.
2. **RTT step-change / delay gradient** — activates GCC's delay-based
   estimator (Kalman filter reacts to RTT *increase*, not absolute RTT).

Without dedicated cases we cannot verify that the QoS state machine responds
correctly when GCC actually reduces its estimate.

## In Scope

- 12 new `gcc_degrade` cases (GD1–GD12) in `sweep_cases.json`.
- All marked `extended:true` — excluded from default gate, run via
  `--include-extended` or `--cases=GD1,...`.
- Group-specific synthetic shaping in `run_cpp_client_matrix.mjs`.
- Monotonicity validation for the new group in `test.synthetic_sweep.mjs`.

## Out Of Scope

- Changing the default 43-case main gate count.
- Updating `docs/qos-status.md` or `docs/plain-client-qos-parity-checklist.md`
  (gate count remains 43).
- Multi-phase / ramping RTT within a single case (not supported by current
  netem harness).

## Acceptance Criteria

### AC-1: Default gate unchanged

The default `filterScenarioCatalog()` call (no `includeExtended`) SHALL
return exactly 43 cases, the same set as before this change.

### AC-2: Extended path includes new cases

`filterScenarioCatalog(scenarios, { includeExtended: true })` SHALL return
55 cases (43 default + 12 GD).

### AC-3: Monotonicity

The `gcc_degrade` group SHALL pass the existing monotonicity validation in
`test.synthetic_sweep.mjs` — harsher loss → equal or worse expected state.

### AC-4: Synthetic shaping

`buildMatrixTestProfile` SHALL apply `qualityLimitationReason: 'bandwidth'`
for `gcc_degrade` cases with ≥5% loss during impairment.

## Non-Functional Requirements

- Performance: no impact on default gate runtime.
- Compatibility: no changes to existing 43-case expectations.

## Edge Cases

- GD cases with mixed impairment dimensions (GD5, GD6, GD8, GD9) may not
  sort monotonically by a single key. The monotonicity test uses `loss` as
  the primary sort key; multi-dimensional cases are acceptable as long as
  higher-loss cases have equal-or-worse expectations.

## Open Questions

- None remaining.
