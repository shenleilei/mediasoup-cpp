## Problem Statement

The current default uplink QoS gate covers 43 non-extended cases. After the synthetic suite was realigned toward GCC-oriented semantics, there is still no dedicated default-gate coverage for clearly degradative GCC-style inputs such as:

- sustained loss at or above the classic GCC reduction regime
- compound bandwidth + loss pressure
- large RTT step changes
- RTT step plus high jitter
- short but material loss bursts

An upstream proposal adds 12 `gcc_degrade` cases (`GD1-GD12`) to `sweep_cases.json`, but without verification and without updating the repository's documented gate contract.

## Goal

Expand the default uplink QoS gate to include 12 new `gcc_degrade` cases and update verification and documentation so the repository's stated gate size and coverage remain truthful.

## In Scope

- Add `GD1-GD12` as non-extended uplink QoS scenarios
- Treat them as part of the default browser/plain-client uplink matrix gate
- Update documentation that currently states the default gate is `43 case`
- Run focused verification for the newly added gate surface on both browser and `cpp-client`
- Record the resulting gate count and coverage clearly

## Out Of Scope

- Re-tuning the synthetic model coefficients again
- Reclassifying existing non-GD cases
- Converting the synthetic suite into physical-E2E verification
- Broad browser matrix methodology refactors unrelated to the new gate cases

## User Scenarios

### Scenario 1: Default gate includes explicit GCC degradation coverage

- GIVEN a default uplink QoS matrix run without `--cases`
- WHEN the scenario catalog is loaded
- THEN the default non-extended gate includes `GD1-GD12`

### Scenario 2: Repository status pages stay truthful

- GIVEN the default gate now contains the original 43 cases plus `GD1-GD12`
- WHEN a reader checks the QoS status or parity docs
- THEN those docs describe the expanded gate size and its coverage accurately

### Scenario 3: New gate surface is verified

- GIVEN the new `gcc_degrade` cases are part of the default gate
- WHEN verification runs
- THEN both browser and `cpp-client` paths have explicit verification evidence for the newly added cases

## Acceptance Criteria

- `sweep_cases.json` contains `GD1-GD12` as non-extended cases in the default catalog
- Documentation that currently says `43 case` or `43 / 43 PASS` is updated where that statement is affected by the new default gate definition
- Focused verification is run for `GD1-GD12` on browser and `cpp-client`
- The change records whether the expanded gate is fully passing or what residual failures remain

## Non-Functional Requirements

- Keep the change narrow: add the cases, update gate-facing docs, and verify the new gate surface
- Do not silently widen the gate without documentation updates
- Do not weaken existing severe bandwidth/loss expectations to make the new group pass

## Edge Cases And Failure Cases

- If browser and `cpp-client` disagree on one or more new `GD` cases, the docs must not claim a clean fully expanded gate pass
- If any `GD` case only works with runner-specific behavior, that must be documented rather than hidden
- If the added cases make the default gate too expensive or unstable, record that explicitly instead of silently keeping them in the default catalog
