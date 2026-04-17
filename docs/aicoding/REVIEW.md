# Review Standard

This file defines how changes should be reviewed.

Use it for:

- self-review before claiming completion
- AI-assisted code review
- human review of pull requests or patches
- branch-to-branch comparison review

## 1. Review Inputs

Before reviewing, read:

- `PROJECT_STANDARD.md`
- `PLANS.md`
- active change docs under `changes/`
- `DELIVERY_CHECKLIST.md`
- relevant accepted specs under `specs/current/`
- the actual diff

If the review is for a branch comparison or pull request, also read:

- the target base branch or merge target
- the full changed file set, not only a partial snippet
- any changed build, config, schema, migration, or workflow files

Treat the documented design standard and implementation plan as review inputs, not optional background material.

## 2. Whole-System Review Rule

Always review from the overall system perspective, not from isolated file snippets.

That means the reviewer should ask:

- what behavior changed at the system level?
- what boundaries are affected upstream and downstream?
- what assumptions changed outside the edited file?
- what can break at runtime even if the local diff looks reasonable?
- what tests, docs, configs, or rollout steps should have changed but did not?

Do not treat review as a style pass over edited lines.

Review the change in the context of:

- system behavior
- module boundaries
- data and contract flow
- operational behavior
- test coverage
- rollout and backward compatibility

Review MUST be performed against the documented design standard and implementation plan.

Do not review against an imagined design that exists only in the reviewer's head.

If the implementation diverges from the documented design standard or implementation plan, call that out explicitly as a finding unless the docs were updated and the deviation was intentionally approved.

## 3. Review Priorities

Review in this order:

1. correctness and regression risk
2. missing or weak verification
3. API, schema, permission, or workflow contract breakage
4. security and data-exposure risk
5. concurrency, reliability, and operational failure modes
6. performance impact on hot paths
7. documentation drift
8. code style or polish issues

Do not spend review energy on stylistic nits before correctness and verification are covered.

## 4. Branch Comparison Review

When reviewing one branch against another branch, compare the branches as complete change sets.

Do not review only the visible line diff in isolation.

Check all of the following:

- what the branch changes at the feature or behavior level
- whether the branch is internally consistent across code, tests, docs, configs, and migrations
- whether the branch introduces partial updates across boundaries
- whether deleted files or deleted tests hide regression risk
- whether the base branch contains assumptions this branch invalidates
- whether merge direction or branch age creates hidden incompatibilities

The output should describe the branch impact globally, then list file-level findings.

## 5. What To Check

### Correctness

- Does the implementation match the requirement or bug intent?
- Does the implementation follow the documented design standard and implementation plan?
- If it differs from the documented design or plan, was that change made explicit and updated in the docs?
- Are edge cases and failure paths handled?
- Does the code preserve existing invariants?

### Tests

- Were unit tests added or updated for changed logic?
- Were regression tests added for bugfixes?
- Were integration or contract checks added for cross-boundary changes?
- Were required repo commands actually run?

### Contracts

- Did APIs, events, permissions, schemas, or state transitions change?
- If they changed, were downstream callers and docs updated?
- Is backward compatibility preserved?

### Operations

- If this fails in production, will logs or signals make diagnosis possible?
- Is rollout or rollback clear enough?
- Are config defaults safe?

### Documentation Currency

- Before every commit, review and update all affected documentation, not only the edited code files.
- Verify that architecture notes, workflow docs, runbooks, specs, test plans, and examples still match the implemented logic.
- Remove or correct stale descriptions when behavior, assumptions, control flow, or operational guidance changed.
- Do not leave outdated or partially true documentation behind for future reviewers or agents.

### Security

- Are auth, authz, secrets, and sensitive data handled correctly?
- Is sensitive data exposed in logs, APIs, or frontend state?

## 6. Review Output Format

For AI reviews, respond in this order:

### Findings

- List findings first, ordered by severity.
- Include file references.
- Explain the impact and why it matters.
- For branch comparison, start from the overall branch impact, then list concrete findings.

### Open Questions

- Call out assumptions that could change the review outcome.

### Verification Gaps

- State missing tests, missing commands, or missing smoke checks.

### Summary

- Keep this short.

If there are no findings:

- say that explicitly
- mention residual risks or missing verification

## 7. Self-Review Before Completion

Before claiming a change is done, the author should ask:

- Does the change match the approved intent?
- Am I reviewing and judging this change against the documented design standard and implementation plan?
- What can regress?
- What still lacks proof?
- What boundary changed?
- What production failure mode have I not considered?
- What documentation became stale or less precise because of this change, and has it been updated before commit?
- If this branch is compared against another branch, what global behavior change does the branch introduce?

If those answers are unclear, the work is not ready to close.
