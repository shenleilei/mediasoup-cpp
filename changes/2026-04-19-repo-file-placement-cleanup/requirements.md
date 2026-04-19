# Requirements

## Summary

Reorganize obviously misplaced repository files so test fixtures live with tests and obsolete generated reports no longer sit beside current primary docs.

## Business Goal

Developers need the repository root and main docs directory to reflect current project structure instead of accumulating test assets and stale generated artifacts in misleading locations.

## In Scope

- moving root-level tracked media test fixtures into a dedicated test fixture directory
- updating code, scripts, and docs that reference the moved media fixtures
- moving obsolete generated report files out of the primary docs surface into archive space
- updating current docs to stop presenting obsolete report locations as active artifacts

## Out Of Scope

- changing build outputs, local runtime artifacts, or ignored directories such as `build/`, `node_modules/`, `recordings/`, or `artifacts/`
- changing the behavior of the tests beyond fixture path updates
- restructuring intentional agent entrypoint files like `AGENTS.md`, `CLAUDE.md`, or `.codex`

## User Stories

### Story 1

As a developer
I want media fixtures used by tests to live under `tests/`
So that the repository root only contains actual project entrypoints and stable top-level assets.

### Story 2

As a developer
I want obsolete generated reports to move out of the main docs surface
So that current workflow docs do not compete with stale artifacts from older entrypoint contracts.

## Acceptance Criteria

### Requirement 1

The system SHALL keep tracked test media fixtures in a dedicated test fixture location.

#### Scenario: fixture lookup

- WHEN tests or helper scripts need the sweep MP4 fixtures
- THEN they SHALL resolve them from a test fixture directory under `tests/`
- AND the project root SHALL no longer be the canonical location for those tracked fixtures

### Requirement 2

The system SHALL keep obsolete generated reports out of the active docs surface.

#### Scenario: current docs browsing

- WHEN a developer browses the main `docs/` directory
- THEN current full-regression and QoS result pages SHALL remain in the active docs surface
- AND obsolete non-QoS generated report artifacts SHALL be archived instead of appearing as current top-level docs

## Non-Functional Requirements

- Maintainability: path changes must reuse existing helpers and update every in-repo reference
- Traceability: moved files should remain tracked through git renames where possible
- Safety: do not move files whose current top-level placement is part of an intentional project convention

## Edge Cases

- test binaries run from the repository root and may rely on relative fixture paths; those callers must be updated together
- historical change docs may still mention the old report location; those historical records should remain untouched if they are intentionally archival

## Open Questions

- None
