# Design

## Context
The remaining matrix failures are not all runtime bugs. Two issues live in the oracle and harness layers:

1. `expectByRunner` currently replaces base expectations instead of extending them.
2. The browser loopback harness ignores case-declared source semantics and always behaves like a camera publisher.

Together these create false failures for `O1` and misleading behavior for traffic-model cases such as `M1`.

## Root Cause

### 1. Runner expectation replacement
`getCaseExpectation()` returns the runner-specific object directly when present. For `O1`, the loopback runner therefore sees only:

```json
{ "maxActionCount": 5 }
```

and loses the base:

- `states`
- `maxLevel`

Then `analyzeResult()` falls back to a default `stable`-only expectation and reports `analysis=过强` even when the state/level bounds were actually acceptable.

### 2. Raw action counting overstates oscillation
The current `actionCount` counts every non-`noop` trace entry, even repeated identical actions on the same level. Oscillation cases need a count of meaningful action changes, not a count of every repeated sample carrying the same command.

### 3. Loopback source mismatch
The browser loopback harness always constructs:

- a camera-style sender snapshot base
- a camera profile
- a high-motion camera-like synthetic track

Traffic-model cases that declare `source: screenshare` or `source: audio` therefore do not actually exercise the intended model.

## Approach

### A. Merge runner expectations
Change `getCaseExpectation()` so runner-specific expectations shallow-merge over the base expectation.

This preserves:

- base state/level bounds
- runner-specific overrides such as `maxActionCount`

### B. Count meaningful action changes
Introduce a helper that collapses consecutive identical non-`noop` actions into a single meaningful action event.

Use this helper in:

- `run_matrix.mjs`
- `run_cpp_client_matrix.mjs`

so oscillation gating reflects actual control changes.

### C. Parameterize loopback source
Pass case `source` into the browser loopback harness and build the correct sender/profile combination:

- `camera`: existing high-motion camera path
- `screenShare`: lower-motion, detail-oriented track plus screen-share profile
- `audio`: audio-source semantics for controller/profile even if the underlying loopback transport still uses the same browser sender primitive

For this bugfix, the critical path is `screenShare` because it directly fixes `M1/M2`.

## Non-goals
- Rewriting the entire synthetic sweep framework.
- Introducing a brand-new oscillation scoring model beyond the meaningful-action count.

## Verification Strategy
- `node --test tests/qos_harness/test.synthetic_sweep.mjs`
- `scripts/run_qos_tests.sh --matrix-cases=M1 matrix`
- `scripts/run_qos_tests.sh --matrix-cases=O1 matrix`
