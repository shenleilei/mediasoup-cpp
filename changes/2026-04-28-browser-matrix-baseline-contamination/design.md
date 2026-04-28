# Bugfix Design

## Context

The browser matrix uses a heuristic to catch leaked netem or startup contamination before the real impairment phase begins. That heuristic is valid for normal sweep/transition cases, but it is wrong for baseline-group cases because those cases intentionally validate degraded baseline conditions.

## Root Cause

`detectBaselineContamination()` currently looks only at the observed baseline state and the baseline network envelope. It does not consider the semantic role of the case itself.

For `B3`, the case definition explicitly expects a degraded weak baseline, but the heuristic still interprets that degradation as infrastructure contamination.

## Fix Strategy

- Extract the helper into a small dedicated module so it can be unit-tested directly.
- Make the helper return `null` immediately for `baseline` group cases.
- Keep the existing contamination behavior unchanged for all other groups.

## Risk Assessment

- Low risk.
- The change only removes a false-positive infrastructure gate from baseline-group cases.
- Non-baseline contamination detection remains intact.

## Verification

- Add a focused Node test covering:
  - `B3` no longer tripping contamination
  - a non-baseline mild-network case still tripping contamination on pre-impairment `recovering`
