# Design

## Context

Several current code changes have outpaced the live Markdown docs:

- `src/` runtime code is now split across `MainBootstrap`, `RuntimeDaemon`, `RoomRegistry.cpp`, `WorkerThread.cpp`, `SignalingServerHttp/Ws/Runtime`, and `RoomService*` collaborators
- Redis-backed integration suites now run against isolated `redis-server` instances instead of assuming a shared Redis on `127.0.0.1:6379`
- `scripts/run_all_tests.sh` and `scripts/run_qos_tests.sh` keep running selected groups after failures, and the non-QoS runner rewrites `docs/non-qos-test-results.md`
- multiple change folders and doc indexes already mention these behaviors, but not all live docs and change records are aligned yet

## Approach

Refresh docs in layers from highest-navigation-value to lowest:

1. top-level navigation and execution docs
   - `README.md`
   - `docs/README.md`
   - `docs/DEVELOPMENT.md`
   - `specs/current/test-entrypoints.md`

2. AI-coding and workflow docs that future agents will read first
   - `AGENTS.md`
   - `docs/aicoding/*`

3. active change docs whose task state or implementation notes drifted from reality
   - especially the active `changes/2026-04-17-*` and `changes/2026-04-18-*` folders tied to current code

4. supporting docs that reference the old `src/` monolith or outdated test assumptions

## Scope Boundary

This refresh should update live, repository-owned docs only.

Do not rewrite:

- vendored docs
- historical archive snapshots
- large domain design docs that are still accurate and unaffected by the current implementation

## Risks

- Broad docs edits can accidentally rewrite stable historical context.
  Mitigation: avoid `docs/archive/` and upstream/vendor trees.

- The current `src` architecture refactor is still an active change, so some docs may need to describe “current split with active refactor” rather than “fully finalized architecture”.
  Mitigation: describe what exists in the tree today and keep forward-looking claims confined to change docs.

- Documentation-only sweeps can become noisy.
  Mitigation: focus on behavior, entrypoints, navigation, and file ownership changes; avoid rephrasing unaffected sections.

## Verification

- inspect the final Markdown diff for only repo-owned live docs
- run shell syntax checks for any touched scripts referenced by docs
- use targeted searches to ensure obsolete guidance such as shared Redis assumptions or old monolithic file references no longer remain in live docs where they would mislead execution
