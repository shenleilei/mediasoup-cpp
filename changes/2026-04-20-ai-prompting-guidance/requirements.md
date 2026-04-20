# Requirements

## Summary

Add a repository-local prompting guidance layer and reusable Copilot/Codex prompt assets that improve day-to-day IDE productivity without weakening the project's existing workflow, review, and verification standards.

## Business Goal

Developers need a consistent way to use AI coding tools inside the repository:

- without re-explaining the same repo context in every chat
- without turning prompt libraries into competing sources of truth
- without weakening requirements, review, or verification gates

## In Scope

- adding a supplemental prompting guide under `docs/aicoding/`
- adding reusable prompt files under `.github/prompts/` for common repo workflows
- adding path-specific Copilot instruction files under `.github/instructions/`
- updating thin entrypoint docs so the new prompting assets are discoverable

## Out Of Scope

- replacing `PROJECT_STANDARD.md`, `PLANS.md`, or `REVIEW.md`
- introducing model-specific API code or runtime integrations
- importing large third-party prompt libraries verbatim into the repository
- treating prompt files as completion gates or durable project memory

## User Stories

### Story 1

As a developer using Copilot or Codex in an IDE
I want reusable repo-specific prompt templates
So that I can start planning, review, testing, and explanation tasks with the right context already attached.

### Story 2

As a maintainer of the repo's AI workflow
I want prompting guidance to live in a supplemental layer
So that hard workflow rules stay in the existing standards and do not drift across tool-specific files.

### Story 3

As a reviewer
I want path-specific instructions for source, tests, and docs
So that code-generation help stays aligned with the expectations of each area of the repository.

## Acceptance Criteria

### Requirement 1

The repository SHALL define a supplemental prompting guide that does not override the existing workflow standards.

#### Scenario: prompting guidance is consulted

- WHEN a developer reads the prompting guide
- THEN it SHALL state that `PROJECT_STANDARD.md`, active change docs, `REVIEW.md`, and `PLANS.md` remain the controlling workflow sources
- AND it SHALL explain what prompting assets are for versus what they are not for

### Requirement 2

The repository SHALL provide reusable prompt files for common AI-assisted development workflows.

#### Scenario: structured change planning

- WHEN a developer uses the planning prompt
- THEN it SHALL direct the tool to classify work, use a change folder for non-trivial work, and map acceptance criteria to verification

#### Scenario: review workflow

- WHEN a developer uses the review prompt
- THEN it SHALL direct the tool to review from the whole-system perspective and output findings before summary

#### Scenario: regression testing

- WHEN a developer uses the regression-test prompt
- THEN it SHALL direct the tool to add or update targeted regression coverage for the changed behavior

#### Scenario: runtime explanation

- WHEN a developer uses the runtime-explanation prompt
- THEN it SHALL direct the tool to explain execution flow, invariants, and failure handling with file references

### Requirement 3

The repository SHALL provide path-specific instructions for major repository areas.

#### Scenario: working in source files

- WHEN Copilot or another supported tool edits source/build files
- THEN source-specific instructions SHALL emphasize lifecycle correctness, reuse, narrow diffs, and test updates for behavior changes

#### Scenario: working in tests

- WHEN Copilot or another supported tool edits test files
- THEN test-specific instructions SHALL emphasize coverage of success, edge, failure, and lifecycle regression paths

#### Scenario: working in docs and instruction files

- WHEN Copilot or another supported tool edits docs or AI instruction files
- THEN docs-specific instructions SHALL emphasize thin wrappers, accurate references, and non-duplication of the core standards

### Requirement 4

The new prompting assets SHALL be discoverable from the existing AI workflow entrypoints.

#### Scenario: reading repo AI docs

- WHEN a developer opens `docs/aicoding/README.md` or `.github/copilot-instructions.md`
- THEN those entrypoints SHALL mention the supplemental prompting layer and the path-specific or reusable prompt assets

## Non-Functional Requirements

- Maintainability: the new prompting guidance must stay additive and must not create a second workflow standard
- Compatibility: the layout should follow current GitHub Copilot repository customization surfaces where practical
- Portability: the guidance should remain useful for Codex and other coding agents even when a specific IDE feature is unavailable
- Safety: prompt templates must not encourage skipping review, tests, or change-document requirements

## Edge Cases

- Some tools may ignore `.github/instructions/` or `.github/prompts/`; the docs must state that these files are helpers, not required gates
- A prompt file may be used without enough context attached; the prompt should direct the user or tool to read the controlling repo docs first
- Documentation-only work may not require the same test commands as code changes; the prompts must not force irrelevant verification

## Open Questions

- None at this time
