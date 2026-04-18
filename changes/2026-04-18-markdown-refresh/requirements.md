# Requirements

## Summary

Refresh the repository-maintained live Markdown documentation so it matches the current codebase, test entrypoints, and active workflow conventions.

## Business Goal

Developers and AI agents need the written documentation to describe the current runtime layout and test workflow accurately, so they can navigate the codebase and execute the right verification steps without relying on stale tribal knowledge.

## In Scope

- repository-maintained live Markdown docs under the repo root, `docs/`, `specs/current/`, and active `changes/`
- documentation indexes, workflow references, and navigation tables
- runtime and test workflow descriptions affected by the current `src/` architecture split, Redis-backed test isolation, keep-going test runners, and generated non-QoS report flow
- change docs that are now inconsistent with the implemented code or verification status

## Out Of Scope

- vendored or upstream docs under `third_party/`
- upstream mediasoup-worker docs under `src/mediasoup-worker-src/`
- historical snapshot docs under `docs/archive/` except for index-level references if needed
- rewriting domain design docs whose content is still accurate and unaffected by current code changes
- changing production behavior as a primary goal

## Acceptance Criteria

### Requirement 1

The live documentation SHALL describe the current runtime module boundaries accurately.

#### Scenario: source navigation

- WHEN a developer reads the top-level or development docs
- THEN they SHALL see the current split of bootstrap, signaling, room-service, registry, and worker-thread responsibilities instead of the previous monolithic file layout

### Requirement 2

The live documentation SHALL describe the current test entrypoints and Redis-backed integration behavior accurately.

#### Scenario: running regression suites

- WHEN a developer follows the documented test workflow
- THEN the docs SHALL match the current keep-going runner behavior, generated non-QoS report behavior, and isolated Redis-backed integration setup

### Requirement 3

Active change documents SHALL remain truthful after the documentation refresh.

#### Scenario: reviewing active work

- WHEN a reviewer reads the active change folders related to the current code changes
- THEN task status, implementation notes, and documentation references SHALL match the implemented state

## Non-Functional Requirements

- Keep edits narrow and traceable to current behavior
- Preserve historical docs as historical snapshots
- Prefer explicit file and command references over vague prose
- Avoid introducing new requirements not already reflected in code or accepted specs
