# TWCC Documentation Refresh

## Problem Statement

The plain-client TWCC / send-side transport-control work now spans transport negotiation, threaded runtime architecture, send-side BWE modules, regression harnesses, generated reports, and accepted specs.

The repository already contains the code and change docs for that work, but the reader-facing architecture and status documents still do not present the current plain-client TWCC path as one coherent story. Some docs still describe the plain client as a mostly single-threaded media loop without the current `NetworkThread` / `SenderTransportController` / send-side BWE main path.

## Goal

Create one reader-facing document that consolidates the TWCC-related branch delta relative to `codex/2026-04-20-functional-refactors`, and update the architecture/status documents that are now stale because of the TWCC send-side path.

## In Scope

- A consolidated TWCC summary document under `docs/`
- Architecture/status doc refresh for the plain-client TWCC runtime and regression/reporting path
- Cross-link updates between the refreshed docs and the stable TWCC A/B report entrypoints
- Change docs for this documentation-only structured change

## Out Of Scope

- Code behavior changes
- New test cases or harness behavior
- Reopening the accepted send-side BWE scope or redesigning the TWCC path itself
- Rewriting historical change docs that already correctly describe the implementation wave

## Acceptance Criteria

1. The repository SHALL contain one reader-facing document that summarizes the TWCC-related branch delta against `codex/2026-04-20-functional-refactors`, covering:
   - transport negotiation / protocol changes
   - plain-client runtime and transport-control changes
   - send-side BWE / probing changes
   - regression/reporting entrypoint changes
2. The refreshed architecture docs SHALL describe the current plain-client threaded runtime in a way that is consistent with the implemented `NetworkThread` / source-worker / send-side TWCC path.
3. The refreshed status docs SHALL link both the consolidated TWCC summary document and the stable generated TWCC A/B report.
4. The updated docs SHALL no longer describe the current plain-client main path as only a single main-thread media loop when discussing the implemented threaded transport path.

## Non-Functional Requirements

- Keep the refreshed docs concise enough to be readable as current-state documentation, not as a replay of every historical review.
- Make the comparison baseline explicit so readers are not confused by the local-vs-remote branch state.
- Preserve the existing document split:
  - high-level repo architecture
  - Linux client architecture
  - status/entrypoint pages
  - detailed TWCC evaluation doc

## Open Questions

- None for this documentation refresh.
