## Context

The synthetic suite now follows a more GCC-oriented interpretation for moderate degradation. The existing 43-case default gate still emphasizes:

- baseline
- bandwidth sweep
- loss sweep
- RTT sweep
- jitter sweep
- transition
- burst

What it does not explicitly cover in the default gate is a dedicated family of "GCC should clearly degrade here" cases.

`GD1-GD12` are intended to fill that gap.

## Decision

Adopt `GD1-GD12` into `tests/qos_harness/scenarios/sweep_cases.json` as non-extended cases, making them part of the default uplink matrix gate.

Then update gate-facing documentation to reflect the new default gate count and coverage.

Verification will be incremental:

- run the newly added `GD1-GD12` on browser
- run the newly added `GD1-GD12` on `cpp-client`
- combine that evidence with the already-established pass state for the existing 43 default cases

## Why This Is Correct

### Why Expand The Gate Instead Of Keeping Them Targeted

These cases are not exploratory edge sentinels like `BW2`.

They represent default, desirable controller behavior under unmistakably degradative GCC-style inputs:

- high enough loss to trigger loss-based reduction
- strong RTT steps
- high jitter combined with RTT change
- compound impairment

That makes them more appropriate for the default gate than for an opt-in extended bucket.

### Why Incremental Verification Is Acceptable

This change adds new cases but does not change the old 43 case definitions or the matrix runner itself.

Therefore the key new verification boundary is:

- whether the 12 added cases pass on browser
- whether the 12 added cases pass on `cpp-client`

Re-running the entire pre-existing 43 browser gate is optional from a coverage standpoint, because the unchanged 43-case evidence already exists and the new risk surface is the newly added group.

### Why Documentation Must Change

The repository currently contains multiple stable statements that the uplink gate is `43 case` and `43 / 43 PASS`.

Once `GD1-GD12` become non-extended default scenarios, those statements become stale even if the code is correct.

## Affected Areas

- `tests/qos_harness/scenarios/sweep_cases.json`
- Uplink QoS status and parity docs
- Coverage docs that enumerate the gate contents
- Generated targeted reports produced by focused browser and `cpp-client` reruns

## Verification Strategy

1. Add `GD1-GD12` to the default catalog on the current branch
2. Run `cpp-client` targeted verification for `GD1-GD12`
3. Run browser targeted verification for `GD1-GD12`
4. Update docs with the new default gate count and verification outcome

## Risks

- One or more `GD` cases may still be too aggressive for current browser or `cpp-client` behavior
- The docs may need to distinguish "default gate definition expanded" from "latest full default run artifact regenerated"
- If browser targeted verification is flaky, the new group may need follow-up stabilization before it can honestly be called part of a stable default gate
