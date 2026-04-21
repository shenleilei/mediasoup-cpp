# Design

## Context

The TWCC-related work landed as a sequence of implementation commits and two structured changes:

- `changes/2026-04-21-plain-client-sender-transport-control/`
- `changes/2026-04-21-livekit-aligned-send-side-bwe/`

Those change folders are accurate for design and verification, but they are not the right reader-facing entrypoint for someone asking:

- what changed on this branch relative to the functional-refactors baseline
- where TWCC was wired in
- how the current threaded plain-client architecture actually works
- where the stable A/B report now lives

## Baseline Choice

The local branch `codex/2026-04-20-functional-refactors` is already ahead of `origin/codex/2026-04-20-functional-refactors`, so a local branch diff would hide most of the TWCC implementation wave.

The consolidated summary document will therefore explicitly use:

- `origin/codex/2026-04-20-functional-refactors`

as the comparison baseline for the full TWCC delta, while explaining why that baseline was chosen.

## Documentation Set

### 1. Consolidated TWCC Delta Document

Add one reader-facing summary document under `docs/` that answers:

- what changed
- why it matters
- what modules and files were affected
- what tests/reporting were added
- what remains intentionally out of scope

This document is the canonical “branch delta” explanation.

### 2. Linux Client Architecture Refresh

Update `docs/linux-client-architecture_cn.md` so it reflects the current accepted threaded main path:

- `PlainClientApp` as bootstrap / runtime selector
- control thread and `WsClient` reader thread
- `NetworkThread` as transport owner
- `SourceWorker` / audio worker data flow
- `SenderTransportController`
- `TransportCcHelpers`
- send-side BWE / probe modules
- local white-box observability and TWCC A/B regression entrypoints

The design intent is to describe current runtime ownership and control flow, not historical migration blockers.

### 3. High-Level Architecture Summary Refresh

Update the plain-client sections of:

- `docs/architecture_cn.md`
- `docs/full-architecture-flow_cn.md`

These docs only need summary-level changes, but they must no longer present the plain client as an unthreaded main-loop-only sender when describing the current main path.

### 4. Status Page Refresh

Update:

- `docs/plain-client-qos-status.md`
- `docs/qos-status.md`

so the current stable status also references:

- the TWCC send-side transport path
- the stable generated TWCC A/B report
- the new consolidated TWCC summary document

## Why Not Reuse An Existing Doc

`docs/twcc-ab-test.md` is intentionally about the A/B harness only.

The change docs under `changes/2026-04-21-*` are intentionally implementation- and review-oriented.

The architecture docs are intentionally current-state docs, not branch-delta docs.

Therefore the repository needs one additional document whose only job is to explain the TWCC-related branch delta coherently.

## Verification Strategy

This is a docs-only change, so verification will focus on:

- diff consistency against the actual branch delta and changed file set
- cross-link consistency between refreshed docs
- `git diff --check` cleanliness
