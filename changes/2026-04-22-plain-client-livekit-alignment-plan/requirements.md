# Requirements

## Problem Statement

The current `PlainTransport C++ client` sender-side QoS implementation was built incrementally and now carries architectural debt:

- duplicated boundary logic across legacy and threaded paths
- mixed sender-stat sources feeding one control pipeline
- partial baseline and source-switch compensation instead of one explicit contract
- transport and policy responsibilities that are not cleanly separated

The user direction for the next phase is explicit:

- do not preserve compatibility with the old implementation shape
- do not optimize for gradual coexistence
- design the target architecture at a higher standard and replace the old path

The repository needs a durable plan for a rewrite-first alignment with the LiveKit-style split:

- transport inner loop
- policy outer loop

with a clear cutover strategy and explicit deletion of obsolete runtime code after the new path is ready.

## Business Goal

Provide a concrete rewrite roadmap so maintainers can:

- replace the current sender QoS runtime path with a cleaner architecture
- avoid spending effort on preserving old compatibility seams
- align the architecture with LiveKit where it matters
- sequence the rewrite in clear implementation waves with an explicit cutover point

## In Scope

- define the target architecture for a rewrite-first LiveKit-style alignment
- define what remains source-of-truth vs observation-only
- define the cutover strategy from old runtime path to new runtime path
- define which old runtime code is expected to be deleted after cutover
- define implementation phases, work packages, and validation strategy
- identify the main modules affected in each phase

## Out Of Scope

- implementing the rewrite itself
- keeping a long-term compatibility layer between old and new sender QoS architectures
- changing accepted behavior in this planning step
- re-running matrix or harness tests as the primary basis for the plan
- upgrading the worker beyond already discussed/landed backports

## User Stories

### Story 1

As a maintainer working on plain-client sender QoS,
I want one implementation plan that explains the target architecture and the migration order,
so that I do not need to infer the next refactor from scattered docs and chat.

### Story 2

As a maintainer comparing this repo with LiveKit,
I want the plan to describe where we should align behaviorally and where we should avoid needless byte-for-byte imitation,
so that the implementation stays pragmatic.

### Story 3

As a maintainer reviewing later refactor waves,
I want the phases and non-goals written down,
so that scope stays controlled and each wave remains reviewable.

### Story 4

As a maintainer doing the rewrite,
I want the plan to say explicitly which old code paths should be retired instead of adapted,
so that we do not carry forward debt by accident.

## Acceptance Criteria

### Requirement 1

The repository SHALL contain a durable plain-client LiveKit-alignment planning document.

#### Scenario: maintainer looks for the roadmap

- WHEN a maintainer opens the plain-client QoS docs
- THEN they can find a dedicated planning document
- AND it explains the target architecture and phased implementation order

### Requirement 2

The planning document SHALL make source-of-truth boundaries explicit.

#### Scenario: maintainer evaluates sender QoS inputs

- WHEN the maintainer reads the plan
- THEN it clearly states which sender-side signals should remain control inputs
- AND which sources should become observation-only

### Requirement 3

The planning document SHALL define a rewrite and cutover strategy, not a compatibility strategy.

#### Scenario: maintainer plans implementation waves

- WHEN the maintainer reads the roadmap
- THEN it states that the new sender QoS architecture is intended to replace the old one
- AND it identifies the major obsolete runtime paths expected to be deleted after cutover

### Requirement 4

The planning document SHALL provide an implementation sequence.

#### Scenario: maintainer prepares the next refactor wave

- WHEN the maintainer reads the roadmap
- THEN the work is broken into concrete phases
- AND each phase names the main modules, goals, and validation expectations

## Non-Functional Requirements

- the document should be implementation-oriented, not an abstract architecture essay
- it should be concise enough to plan from, but concrete enough to prevent scope drift
- it should separate required rewrite work from optional future improvements

## Edge Cases And Failure Cases

- some current runtime helpers or test-only hooks may need to be relocated rather than preserved in place
- deleting old sender QoS runtime code too early can temporarily strand required transport signals if the replacement path is incomplete
- the plan should not assume that all LiveKit internals must be cloned exactly
