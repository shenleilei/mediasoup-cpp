# Requirements

## Summary

Refresh the repository review standard so it absorbs strong review practices from high-signal C++ and infrastructure projects without replacing the repository's existing house style.

## Business Goal

The repository already has a strong whole-system review standard, but it can better guide C++ backend review in four areas:

- when reviewers should require smaller, more reviewable changes
- when code review is insufficient without prior design consensus
- how reviewer coverage should match domain risk
- what higher-risk C++ backend changes should prove before approval

## In Scope

- strengthening `docs/aicoding/REVIEW.md`
- aligning `docs/aicoding/DELIVERY_CHECKLIST.md` with the refreshed review expectations
- aligning the reusable review prompt under `.github/prompts/`
- recording the change in a Structured Change folder

## Out Of Scope

- introducing CODEOWNERS, maintainers, or enforcement bots
- defining branch protection or GitHub workflow automation
- replacing the repository's existing review model with another project's governance model
- adding new build targets, sanitizer scripts, or benchmark harnesses

## User Stories

### Story 1

As a reviewer of C++ backend changes
I want the review standard to tell me when to push back on oversized or mixed-scope diffs
So that risky changes remain reviewable and regression risk stays visible.

### Story 2

As a reviewer of protocol, concurrency, lifecycle, or performance-sensitive changes
I want the standard to require the right expertise and risk-based verification
So that approval does not rely on superficial code reading alone.

### Story 3

As an author
I want the review standard to clarify when code review is not enough without prior design work
So that major architecture, API, and protocol changes are discussed at the right level before approval.

## Acceptance Criteria

### Requirement 1

The review standard SHALL allow reviewers to reject or pause oversized or mixed-scope changes until reviewability is restored.

#### Scenario: change is too large or mixes unrelated concerns

- WHEN a change combines functional behavior, large refactors, renames, formatting churn, or broad file movement
- THEN `REVIEW.md` SHALL state that the reviewer may require the change to be split or its review strategy to be made explicit

### Requirement 2

The review standard SHALL describe when code review must escalate back to documented design intent.

#### Scenario: significant design-impacting change

- WHEN a change materially affects architecture, protocols, public contracts, workflows, or downstream behavior
- THEN `REVIEW.md` SHALL state that code review is not sufficient by itself
- AND it SHALL require review against documented design intent before approval

### Requirement 3

The review standard SHALL require reviewer coverage to match the risk area being changed.

#### Scenario: reviewer lacks full domain expertise

- WHEN a reviewer is not qualified for concurrency, protocol, security, build, or performance-critical aspects of the change
- THEN `REVIEW.md` SHALL require either qualified reviewer involvement or an explicit review-scope limitation and gap

#### Scenario: review approval

- WHEN a change is approved
- THEN `REVIEW.md` SHALL require explicit approval and acknowledgement of substantive reviewer feedback rather than silent approval

### Requirement 4

The review standard SHALL define risk-based verification expectations for higher-risk C++ backend work.

#### Scenario: high-risk runtime change

- WHEN a change affects memory ownership, parsing, syscalls, concurrency, crash recovery, or performance-sensitive paths
- THEN `REVIEW.md` and `DELIVERY_CHECKLIST.md` SHALL require verification evidence proportional to the risk
- AND that evidence SHALL mention applicable sanitizer-like checks, crash or recovery testing, stress or lifecycle testing, or benchmark evidence when relevant

### Requirement 5

The aligned reusable review prompt SHALL remain consistent with the refreshed review standard.

#### Scenario: using the repo review prompt

- WHEN a developer uses `.github/prompts/review-whole-system.prompt.md`
- THEN it SHALL direct the tool to check reviewability, missing expertise, missing design-level review, and missing high-risk verification evidence

## Non-Functional Requirements

- Maintainability: the refreshed standard must remain a house style, not a pasted bundle of external policies
- Compatibility: the guidance must fit the repository's existing `PROJECT_STANDARD.md`, `PLANS.md`, and `REVIEW.md` hierarchy
- Practicality: the standard must require stronger evidence for higher-risk C++ work without inventing repo commands that do not exist
- Clarity: the new rules must be easy to apply during human and AI-assisted review

## Edge Cases

- A repository may not have a dedicated sanitizer or benchmark target for every risk area; the standard should require equivalent evidence or an explicit gap, not pretend the risk does not exist
- Some review assignments may intentionally cover only part of a change; the standard should permit that if the reviewed scope is stated explicitly
- Large emergency fixes may still need rapid landing; the standard should prefer documented review strategy and follow-up verification over pretending the risk disappeared

## Open Questions

- None at this time
