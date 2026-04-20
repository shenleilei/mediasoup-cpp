# Delivery Checklist

Use this checklist before merging any behavior-changing work.

## Scope And Traceability

- [ ] The change is classified correctly as Quick Task or Structured Change
- [ ] Requirements or bug intent are clear and stable enough for implementation
- [ ] Significant architecture, protocol, public-contract, or workflow changes were reviewed against documented design intent, not only the code diff
- [ ] Every acceptance criterion maps to at least one verification activity
- [ ] Out-of-scope items are explicitly excluded
- [ ] Oversized or mixed-scope changes were split or their review strategy and residual risk were documented explicitly

## Tests And Verification

- [ ] New or changed business logic has unit tests for success, edge, and failure paths
- [ ] Cross-module, cross-service, API, DB, queue, or workflow changes have integration tests
- [ ] Bugfixes include a regression test for the original defect
- [ ] Existing affected tests were updated rather than silently bypassed
- [ ] Higher-risk C++ backend changes have evidence proportional to risk, such as sanitizer-like, crash-recovery, stress, or performance checks when applicable
- [ ] Lint passes
- [ ] Type-check passes
- [ ] Build or package checks pass

## Contracts And Compatibility

- [ ] API or event contract changes are documented and tested
- [ ] Schema or migration changes are verified
- [ ] Backward compatibility is preserved or the breaking change is explicitly approved
- [ ] Rollback or mitigation steps exist for risky changes

## Operations

- [ ] Logs, metrics, traces, or alerts were added or updated where production debugging will matter
- [ ] Configuration changes have sane defaults and safe rollout behavior
- [ ] Performance-sensitive changes were measured, not assumed
- [ ] Security-sensitive changes were reviewed for auth, authz, secrets, and data exposure

## Knowledge Update

- [ ] `REVIEW.md` was used for self-review or code review
- [ ] Review considered whole-system impact and not only local file edits
- [ ] For branch or PR review, the full change set was reviewed against the base branch
- [ ] Reviewer coverage matched concurrency, protocol, security, build, or performance risk areas, or any gap was recorded explicitly
- [ ] All affected documentation was reviewed before commit and updated where logic, behavior, assumptions, or procedures changed
- [ ] `specs/current/` reflects accepted behavior
- [ ] Change docs and tasks reflect the final implementation truthfully
- [ ] Follow-up debt and rollout notes are recorded explicitly
