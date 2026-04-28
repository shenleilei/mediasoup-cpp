# Requirements

## Summary

Perform a whole-repository code review and record a formal review report in project documentation.

## Business Goal

Provide an actionable, evidence-based repository health snapshot so maintainers can prioritize correctness and reliability fixes with clear traceability.

## In Scope

- Whole-system static review across the repository (with focus on runtime-critical code in `src/`).
- Review findings ordered by severity with file references and concrete impact.
- Include explicit open questions and verification gaps.
- Record the report in a durable documentation file in the repository.

## Out Of Scope

- Implementing fixes for review findings.
- Large-scale refactors unrelated to producing the review report.
- Changing accepted behavior specs.

## User Stories

### Story 1

As a maintainer
I want a repository-wide review report with concrete file-level findings
So that I can schedule and execute risk-reduction work efficiently.

## Acceptance Criteria

### Requirement 1

The review output SHALL follow `docs/aicoding/REVIEW.md` output order and include findings first.

#### Scenario: Findings formatting

- WHEN the report is generated
- THEN findings are sorted by severity and each finding contains file location and impact explanation.

### Requirement 2

The review output SHALL document verification evidence and known coverage limits.

#### Scenario: Verification transparency

- WHEN build/test checks are run for this review
- THEN the report includes commands executed, observed outcomes, and any missing coverage.

### Requirement 3

The report SHALL be saved in-repo as a documentation artifact.

#### Scenario: Durable handoff

- WHEN the review is complete
- THEN a markdown report file exists under `docs/` and can be used as follow-up input.

## Non-Functional Requirements

- Performance: Review activities should avoid unnecessary full-history churn and focus on current code state.
- Reliability: Findings must be evidence-based and reproducible from current repository content.
- Security: Security-impacting issues discovered during review must be explicitly marked.
- Compatibility: Report format should align with existing repository review standards.
- Observability: Verification commands and outcomes must be captured in the report.

## Edge Cases

- Existing local workspace may contain unrelated uncommitted changes.
- Some test suites may be unavailable or too heavy to execute in the current run.

## Open Questions

- None.
