## Context

The refined synthetic QoS calibration intentionally moved the `cpp-client` synthetic suite closer to common WebRTC GCC intuition:

- loss below the classic GCC reduction regime should not force warning/congestion
- absolute RTT alone should be weaker than before
- jitter should be smoothed instead of treated as raw impairment

After that calibration landed, six expectations in `sweep_cases.json` became stricter than the synthetic suite's intended GCC-oriented semantics.

This change realigns those expectations instead of re-introducing stronger synthetic forcing.

## Decision

Update only the six failing expectations:

- `B3`
- `R4`
- `R5`
- `T6`
- `T7`
- `S3`

The new expectations should allow outcomes that are consistent with common GCC behavior:

- `B3`: allow `stable` or `early_warning`
- `R4`: allow `stable` or `early_warning`
- `R5`: allow `early_warning` or `congested`
- `T6`: allow `stable` or `early_warning`
- `T7`: allow `stable` or `early_warning`
- `S3`: allow `stable` or `early_warning`

## Why This Is Correct

### GCC-Oriented Semantics

The synthetic suite is not a physical-E2E suite. Its purpose is to approximate controller behavior under a GCC-like interpretation.

For the six failing cases:

- loss remains very low (`0.1%` to `0.5%`)
- utilization remains high (`~0.82` to `~0.88`)
- `qualityLimitationReason` remains `none`
- only RTT/jitter are elevated, often without strong congestion corroboration

Under common GCC intuition, these conditions are not strong enough to require persistent `warning` or `congested` outcomes.

### Why Not Re-Tune The Synthetic Model Again

The newly adopted calibration already has:

- JS model tests
- C++ convergence tests
- synthetic pipeline/controller tests

Re-strengthening the model just to satisfy these six expectations would likely move the suite away from the stated GCC-oriented direction and re-introduce hidden forcing.

Updating the expectations is narrower and keeps the synthetic model internally coherent.

## Scope Boundaries

This change does **not**:

- change synthetic model coefficients
- re-introduce RTT forcing or additional synthetic overrides
- reinterpret the suite as physical-E2E verification

This change only updates case expectations to match the accepted synthetic suite semantics.

## Verification Strategy

1. Update `sweep_cases.json`
2. Re-run the 43 targeted `cpp-client` QoS cases
3. Confirm all 43 pass without changing the synthetic model again
