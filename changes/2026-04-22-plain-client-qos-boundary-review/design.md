# Bugfix Design

## Context

This is a documentation-only review artifact, not a runtime fix.

The goal is to convert recent static analysis about plain-client QoS risks into a durable document that future implementation work can reference directly.

## Root Cause

The repository already has:

- architecture docs
- status docs
- TWCC/BWE docs
- generated result docs

What it does not have is a focused “code/principle risk review” document for plain-client QoS boundaries.

That gap matters because several subtle issues live between layers:

- worker stats semantics
- server `getStats` parsing
- local RTCP counters
- QoS baseline and delta logic
- legacy/threaded duplication

Without a dedicated review document, those findings remain easy to lose or re-derive incorrectly.

## Fix Strategy

Add one new plain-client QoS review document under:

- `docs/qos/plain-client/plain-client-qos-boundary-review_cn.md`

and link it from:

- `docs/qos/plain-client/README.md`

The document should prioritize:

1. clear boundary-contract findings
2. explicit file references
3. separation between confirmed issues and lower-confidence audit targets
4. concrete next-action directions without pretending the fixes are already implemented

## Document Structure

Recommended sections:

- purpose and scope
- overall conclusion
- high-confidence findings
- lower-confidence audit targets
- what existing tests/results do and do not prove
- recommended next fixes

## Risk Assessment

- The document could overstate speculative concerns as confirmed defects.
- The document could duplicate status/architecture material instead of focusing on boundary risk.

## Mitigation

- label confidence explicitly
- keep the focus on code/principle mismatches
- avoid repeating broad architecture material already covered elsewhere

## Test Strategy

- `git diff --check`
- manual read-through
- confirm the new doc is reachable from the plain-client QoS status page

## Observability

- not applicable for this docs-only step

## Rollout Notes

- no runtime rollout impact
- later code fixes can reference this document as the problem statement and risk inventory
