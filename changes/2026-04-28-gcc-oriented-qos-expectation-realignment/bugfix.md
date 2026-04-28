## Symptom

After adopting the refined synthetic QoS calibration, the targeted 43-case `cpp-client` uplink QoS matrix regressed from full pass to 37/43 pass.

The six failing cases are:

- `B3`
- `R4`
- `R5`
- `T6`
- `T7`
- `S3`

All six fail with `analysis=过弱`.

## Reproduction

Run:

```bash
node tests/qos_harness/preflight_netem_guards.mjs --iface=lo
node tests/qos_harness/run_cpp_client_matrix.mjs --cases=B1,B2,B3,BW1,BW3,BW4,BW5,BW6,BW7,L1,L2,L3,L4,L5,L6,L7,L8,R1,R2,R3,R4,R5,R6,J1,J2,J3,J4,J5,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,S1,S2,S3,S4
```

## Observed Behavior

The six failing cases remain `stable/L0` or only reach `early_warning/L1`, even though their expectations require `early_warning` or `congested`.

The refined synthetic calibration produces impairment inputs such as:

- `B3`: `util=0.834`, `RTT=76ms`, `loss=0.5%`, `jitter=9`
- `R4`: `util=0.879`, `RTT=218ms`, `loss=0.1%`
- `R5`: `util=0.818`, `RTT=294ms`, `loss=0.1%`
- `T6`: `util=0.879`, `RTT=218ms`, `loss=0.1%`
- `T7`: `util=0.873`, `RTT=39ms`, `jitter=30`, `loss=0.1%`
- `S3`: `util=0.861`, `RTT=240ms`, `loss=0.1%`

## Expected Behavior

Under a GCC-oriented interpretation:

- stable high RTT alone should not require warning/congested
- low loss (`0.1%` to `0.5%`) should not be treated as congestion evidence
- short bursts with stable delay should be allowed to remain `stable`

The six case expectations should align with common GCC behavior instead of assuming that absolute RTT alone implies congestion severity.

## Suspected Scope

- `tests/qos_harness/scenarios/sweep_cases.json`
- QoS expectation documentation that explains the synthetic suite's GCC-oriented semantics

## Known Non-Affected Behavior

- Low-bandwidth severe cases (`BW3+`, `T8+`) still reach `congested`
- High-loss severe cases (`L5+`) still reach `congested`
- Synthetic controller calibration and pipeline unit tests are already green

## Acceptance Criteria

- The six case expectations are updated to match GCC-oriented synthetic semantics
- The 43 targeted `cpp-client` QoS cases pass with the refined synthetic calibration
- Documentation explicitly states that these expectations follow GCC-style behavior rather than conservative product-level warning policy

## Regression Expectations

- Existing severe bandwidth/loss cases must remain unchanged
- The change must not reintroduce legacy RTT forcing such as the removed `B3 RTT=230ms` override
