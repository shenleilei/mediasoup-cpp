# Design

## Context

The current `docs/aicoding/REVIEW.md` is already strong on:

- whole-system review
- lifecycle, protocol, and concurrency analysis
- verification gaps
- documentation drift

What it does not state strongly enough yet is:

- reviewers may require smaller, more reviewable diffs
- some changes need documented design consensus before code review can close
- reviewer coverage should match the risk area being changed
- higher-risk C++ backend changes need stronger verification evidence than ordinary business logic changes

External references influenced this refresh:

- Google eng-practices for review scope, qualified reviewers, and small-change reviewability
- LLVM for significant-change review, explicit approval, comment acknowledgement, and RFC triggers
- Envoy for routing review to the right expertise instead of assuming one reviewer covers all risk areas
- RocksDB for risk-based validation of complex C++ changes
- gRPC proposals for design-up-front treatment of significant API or protocol changes
- MongoDB server design guidelines for C++ class and header review heuristics

The repository should absorb those patterns, not adopt their governance structures directly.

## Proposed Approach

### 1. Strengthen `REVIEW.md` with reviewability and escalation rules

Add explicit sections that:

- empower reviewers to require smaller or split changes when reviewability is poor
- state that code review is not enough for some architectural, protocol, or public-contract changes unless documented design intent exists

This matches the repository's existing change-doc workflow rather than introducing a separate RFC system.

### 2. Add reviewer-coverage and approval-discipline rules

Add rules that:

- require qualified reviewer involvement or explicit scope limitations for specialized risk areas
- require explicit approval rather than silence
- require authors to acknowledge substantive reviewer feedback

This sharpens review discipline without depending on maintainers, CODEOWNERS, or repository automation.

### 3. Add C++ backend-specific review heuristics

Extend `What To Check` with:

- C++ API and class-design questions
- higher-risk verification expectations for memory, concurrency, crash recovery, and performance-sensitive paths

The guidance should ask for evidence proportional to risk but avoid naming repo commands that may not exist.

### 4. Align delivery and prompt layers

Update:

- `docs/aicoding/DELIVERY_CHECKLIST.md`
- `.github/prompts/review-whole-system.prompt.md`

This keeps the review standard, merge checklist, and AI review prompt aligned.

## Alternatives Considered

- Copy another project's review policy wholesale
  - Rejected because those policies depend on maintainer structures, tooling, and governance that this repository does not share

- Only update the review prompt
  - Rejected because the source of truth must remain `docs/aicoding/REVIEW.md`

- Add mandatory sanitizer or benchmark commands directly into the review standard
  - Rejected because the repository does not currently define one standard command for every high-risk verification mode

## Modules And Responsibilities

- `docs/aicoding/REVIEW.md`: source-of-truth review standard
- `docs/aicoding/DELIVERY_CHECKLIST.md`: pre-merge checklist aligned with refreshed review expectations
- `.github/prompts/review-whole-system.prompt.md`: reusable prompt consistent with the review standard
- `changes/2026-04-20-review-standard-refresh/*`: change intent and execution record

## Data And State

- No runtime behavior changes
- No API or schema changes
- No new persistent data

## Interfaces

- Developer-facing documentation and prompting assets only

## Failure Handling

- If some verification evidence is unavailable in a developer environment, the standard should require that gap to be recorded explicitly
- If a review is intentionally partial, the reviewed scope and missing expertise should be stated explicitly

## Security Considerations

- No runtime security behavior changes
- The refreshed review standard should strengthen how security-sensitive changes are reviewed

## Testing Strategy

- `git diff --check`
- inspect the final changed-file set for consistency between `REVIEW.md`, `DELIVERY_CHECKLIST.md`, and the reusable review prompt
- self-review the final diff against the refreshed `REVIEW.md` to ensure the change did not create instruction drift

## Observability

- Not applicable for runtime behavior

## Rollout Notes

- Additive standards refresh only
- No automation or enforcement changes
- Teams can apply the stronger review rules immediately once the docs land
