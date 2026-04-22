# Requirements

## Problem Statement

The repository currently depends on an upstream `mediasoup-worker` baseline around `3.14.6`, but recent upstream releases introduced both valuable bugfixes and upgrade hazards. The analysis currently exists only in chat and ad hoc inspection, which makes future worker upgrade or backport work easy to misremember.

The project needs a durable repository-local document that explains:

- what worker baseline the repo currently uses
- what local worker patch footprint exists today
- which recent upstream fixes matter most to this repo
- which upstream release changes are breaking for the current control-plane integration
- which local patches likely need to be carried forward and which do not

## Business Goal

Give maintainers a single document they can consult before:

- backporting selected upstream worker fixes
- upgrading the vendored `mediasoup-worker` submodule
- changing local worker build or mirror policy
- deciding whether a given upstream release should be consumed whole or cherry-picked partially

## In Scope

- document the current local worker baseline and local patch footprint
- summarize the recent upstream `mediasoup` worker releases relevant to this repo
- explain the observed upgrade blockers in protocol/FBS, build tooling, and local mirror handling
- explain the `3.19.9` RTCP `packets_lost` fix in implementation terms
- explain the `3.19.15` duplicate RTP packet fix in implementation terms
- rank the currently discussed upstream bugfixes by relevance to this repo
- record a recommended near-term strategy:
  - targeted backports vs full worker upgrade

## Out Of Scope

- performing the worker upgrade itself
- backporting any worker fix in this change
- changing `setup.sh`, worker build scripts, or FBS bindings
- modifying accepted runtime behavior or specs

## User Stories

### Story 1

As a maintainer evaluating a worker upgrade,
I want one repository-local assessment doc,
so that I do not need to reconstruct the upgrade risks from chat history.

### Story 2

As a maintainer deciding whether to cherry-pick an upstream fix,
I want the fix priority and compatibility risks written down,
so that I can choose between partial backport and full upgrade intentionally.

### Story 3

As a maintainer debugging RTCP loss stats,
I want the `3.19.9` signed-loss fix explained against the actual worker code path,
so that I understand why the fix matters to QoS and stats correctness.

## Acceptance Criteria

### Requirement 1

The repository SHALL contain a durable worker-upgrade assessment document under `docs/platform/`.

#### Scenario: maintainer looks for worker upgrade guidance

- WHEN a maintainer opens platform docs
- THEN they can find a dedicated worker upgrade assessment document
- AND the document does not rely on chat history to explain the current findings

### Requirement 2

The assessment document SHALL describe the current local baseline and patch footprint.

#### Scenario: maintainer checks current starting point

- WHEN the maintainer reads the assessment
- THEN it states the current vendored worker tag/commit relationship
- AND it states the locally carried worker patch areas

### Requirement 3

The assessment document SHALL distinguish bugfix value from upgrade cost.

#### Scenario: maintainer compares targeted backport against full upgrade

- WHEN the maintainer reads the assessment
- THEN it identifies high-value upstream fixes discussed in this investigation
- AND it explains which upstream changes are protocol/build breaking for the current repo
- AND it recommends whether each discussed item is better treated as a backport candidate or part of a larger upgrade wave

### Requirement 4

The assessment document SHALL explain the `3.19.9` RTCP loss-stat fix precisely.

#### Scenario: maintainer wants to understand `packets_lost`

- WHEN the maintainer reads the `3.19.9` section
- THEN the document explains why RTCP `total lost` is signed
- AND it explains how `expected` and `received` are computed in worker code
- AND it explains how unsigned propagation can corrupt stats and QoS interpretation

## Non-Functional Requirements

- the document must be concise enough to scan but specific enough to support implementation planning
- all repository-local claims must point to concrete local files or commits
- upstream release/fix references must identify exact release numbers and bugfix subjects

## Edge Cases And Failure Cases

- local worker patch behavior may differ from upstream because the current submodule already includes one local commit past `3.14.6`
- some upstream release changes are not directly valuable to this repo today but still matter because they change protocol or build assumptions
- Aliyun mirror preference must not be documented as available when the required artifact was not actually found
