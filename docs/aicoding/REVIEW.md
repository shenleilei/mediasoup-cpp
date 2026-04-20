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

For stateful, retry-prone, or protocol-heavy changes, review MUST explicitly trace:

- first-time setup or happy-path execution
- repeated call, retry, reconnect, or replacement of the same resource
- teardown, close, unregister, or cleanup behavior
- partial failure and rollback behavior
- counterpart interpretation of emitted protocol, codec, or wire-level values

Review MUST be performed against the documented design standard and implementation plan.

Do not review against an imagined design that exists only in the reviewer's head.

If the implementation diverges from the documented design standard or implementation plan, call that out explicitly as a finding unless the docs were updated and the deviation was intentionally approved.

### Anti-Anchoring Rule

- Do not anchor the review only on the currently reported bug cluster, active change narrative, or the files already suspected in chat or docs.
- For non-trivial changes and branch-wide reviews, perform at least one deliberate pass over shared infrastructure or foundational modules that could invalidate the change indirectly.
- Do not assume untouched base-layer code is safe just because the current diff did not edit it.
- If review coverage is intentionally limited, state that limit explicitly instead of silently treating uninspected areas as safe.

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

## 4. Change Size And Reviewability

Reviewers are allowed to push back on a change that is too large or too mixed to review reliably.

Reviewers should ask:

- does this diff stay small enough to reason about end to end?
- does it mix behavior change with mechanical refactor, rename, formatting, or file-move churn that should usually be separate?
- would a preparatory refactor, test move, or cleanup pass make the behavior change easier to review if landed separately?
- if the change cannot be split safely, did the author explain why and make the review strategy explicit?

Small, self-contained changes are preferred because they improve correctness review, rollback clarity, and bisectability.

If a reviewer cannot confidently reason about the change because of its breadth, that is a valid review finding, not a process nit.

## 5. Branch Comparison Review

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

### Required Coverage Beyond The Diff

- Branch comparison review MUST include targeted inspection of foundational modules, shared helpers, or base-layer interfaces affected indirectly by the branch behavior, even when those files are not the most visible part of the diff.
- When branch scope is broad or risk-heavy, the review output MUST state what areas were inspected and what areas were not inspected deeply.
- If a likely-risk area was not inspected deeply enough, report that as a verification gap instead of treating it as safe by omission.

## 6. Reviewer Coverage And Approval Discipline

Review coverage should match the risk areas touched by the change, not only the visible ownership of files.

- If you are not qualified to judge a concurrency, protocol, security, build, crash-recovery, or performance-critical aspect of the change, say so.
- For specialized risk areas, require additional qualified review or record the missing expertise as a review gap.
- Partial reviews are acceptable only when the reviewed scope is stated explicitly.
- Approval must be explicit. Silence, inactivity, or an unanswered thread is not approval.
- Authors should acknowledge every substantive reviewer comment with a code change, a reasoned reply, or an explicit follow-up item accepted as debt.
- Final approval should reflect the latest diff and prior discussion, not only the first patch version.

## 7. When Code Review Is Not Enough

Code review does not replace documented design review for significant changes.

- If a change materially affects architecture, protocols, public APIs or events, schemas, operational workflows, or rollback assumptions, review should confirm that requirements and design docs exist and are still accurate before approval.
- If code review reveals unresolved design disagreement or missing design intent, stop approval and send the change back to the change-doc or design stage.
- Emergency changes may land with narrower upfront design docs only if risk, mitigation, and follow-up verification are explicit.

## 8. What To Check

### Correctness

- Does the implementation match the requirement or bug intent?
- Does the implementation follow the documented design standard and implementation plan?
- If it differs from the documented design or plan, was that change made explicit and updated in the docs?
- Are edge cases and failure paths handled?
- Does the code preserve existing invariants?

### Failure Paths And Defensive Checks

- If the code depends on a library, syscall, parser, allocator, or external API, are failure returns, null results, short reads/writes, timeout paths, and thrown exceptions handled explicitly?
- For multi-step initialization, what happens if step N fails after steps 1 through N-1 already allocated, registered, opened, or started resources?
- For shutdown or teardown, is cleanup ordering still correct when only part of the system started successfully or when an earlier cleanup step already failed?
- Did the review explicitly check for null dereference, stale handles, use-after-close, partially initialized state, and dropped-ownership patterns instead of assuming the happy path?

### Lifecycle, Re-entry, And Replacement

- If the same API or workflow is invoked twice, what happens on the second call?
- If a client retries, reconnects, or replaces an existing resource, is the old runtime resource explicitly closed and unregistered?
- If an owning pointer, handle, or registry entry is overwritten, does that trigger the required cleanup, or does it merely drop local ownership?
- Are caches, reverse indexes, observer lists, and ownership maps updated when resources are replaced or closed?
- Is cleanup behavior verified for both success and partial-failure paths?

### Wire And Protocol Semantics

- For protocol, RTCP/RTP, codec, or schema changes, does the declared contract match the actual emitted and consumed runtime behavior end to end?
- Are field width, signedness, units, timebase, and timestamp semantics checked against the RFC/spec and the counterpart implementation?
- If multiple codec variants exist, is the selected advertised variant the same one the runtime actually produces or expects?
- Did the review follow the contract across all affected layers instead of stopping at a single file that "looks right" locally?

### Serialization, Buffer, And Lifetime Semantics

- If buffers, builders, offsets, views, spans, or pointers are constructed in one context and consumed in another, are their ownership and lifetime rules still valid at the point of use?
- Does any clear, reset, move, resize, or reallocation invalidate data that is later reused?
- Are callback captures, observer registrations, and deferred lambdas holding objects safely, or can they outlive the owning state?
- For schema, IPC, or message-building code, is the body built under the same lock, builder, ownership, and lifetime assumptions as the enclosing message?

### Concurrency And Thread Boundaries

- What shared state is touched from callbacks, worker threads, event loops, timers, signal handlers, or teardown paths?
- Are close or shutdown flags, once-only transitions, and lifecycle guards synchronized correctly across threads?
- Could a callback race with destruction, close, unregister, pointer reset, or resource replacement?
- Does the code rely on thread-affinity assumptions that are implicit rather than enforced or documented?

### Metrics, Fallbacks, And Baselines

- If the same metric can come from multiple sources, do those sources share the same units, counter semantics, reset behavior, and baseline?
- When the implementation falls back from one stats source to another, are delta calculations still valid across the transition?
- Are stale-data, unavailable-data, and recovered-data transitions reviewed explicitly rather than assumed safe?

### Reuse And Refactoring

- Did the change reuse existing code paths, helpers, utilities, and abstractions where practical?
- Does the diff introduce avoidable duplication or another parallel implementation of the same behavior?
- When the change exposed repeated logic or brittle structure, should a small safe refactor have been done as part of the work?
- If refactoring was deferred, is that an explicit tradeoff rather than accidental duplication debt?

### Simplicity And Scope

- Is the change as small and self-contained as practical for the behavior being changed?
- Were large refactors, renames, or formatting-only churn separated from behavior changes when practical?
- Does the implementation solve the current requirement without speculative abstractions or over-engineering?
- Does the change improve code health or at least avoid making the surrounding area harder to understand and modify?

### Naming, Comments, And Consistency

- Are names explicit, predictable, and consistent across code, tests, configs, and docs?
- Is the same concept described with one name instead of multiple competing names?
- Do comments explain why, constraints, invariants, or tradeoffs rather than restating what the code already says?
- Does the change follow existing local conventions unless there is a clear reason to introduce a better shared pattern?

### C++ API And Class Design

- Are public headers exposing only what callers need, or leaking internal implementation details unnecessarily?
- Is inheritance justified, or would composition or delegation be simpler and safer here?
- Are ownership, copy, move, borrowing, and destruction semantics explicit enough to prevent misuse across module boundaries?
- If a class or component is difficult to test without broad integration scaffolding, does that reveal avoidable coupling or a missing boundary?
- Is `friend`, global mutable state, or hidden cross-object coupling introduced without a clear reason?

### Errors And Dependencies

- Are failures and edge cases handled explicitly instead of being silently ignored?
- If failures are intentionally suppressed, is that decision obvious and justified?
- Did the change avoid unnecessary new dependencies, hidden coupling, and extra indirection?
- If a new abstraction or dependency was introduced, is it justified by real repeated need or boundary clarity?

### Tests

- Were unit tests added or updated for changed logic?
- Were regression tests added for bugfixes?
- Were integration or contract checks added for cross-boundary changes?
- Were required repo commands actually run?
- If the change adds, renames, moves, splits, or otherwise rehomes a test case, test suite, or test binary, did the review verify that `scripts/run_all_tests.sh` still covers it directly or through its delegated entrypoints?
- Do not assume a new case is covered just because it compiles or passes when run manually; explicitly check for stale filters, missing group wiring, or renamed binaries/scripts that would leave `scripts/run_all_tests.sh` incomplete.
- For stateful resources, do tests cover repeated call, reconnect, retry, replacement, and cleanup paths?
- For protocol-heavy changes, do tests cover field encoding/decoding, signedness, unit conversion, and timestamp semantics?
- For metrics or state machines, do tests cover source switching, counter resets, and fallback transitions?
- For parser, allocator, syscall, low-level memory, or undefined-behavior-sensitive C++ changes, did the review require sanitizer-like or memory-safety evidence when the repo or local environment supports it?
- For concurrency, teardown, or lifecycle changes, did the review require repeated-run, stress, race-focused, or long-lived verification beyond one happy-path execution?
- For crash-recovery, process supervision, failover, or restart-sensitive behavior, did the review require fault-injection or crash-recovery evidence where applicable?
- For hot-path or allocation-sensitive changes, did the review require benchmark or before-versus-after performance evidence instead of intuition?
- Do not use total test count or broad coverage claims as a proxy for risk coverage; map tests to the specific failure modes introduced by the change.

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

## 9. Review Output Format

For AI reviews, respond in this order:

### Findings

- List findings first, ordered by severity.
- Include file references.
- Explain the impact and why it matters.
- For branch comparison, start from the overall branch impact, then list concrete findings.
- Call out oversized mixed-scope diffs, missing design-stage review, or missing qualified reviewer coverage when those process defects materially reduce confidence in the change.
- Separate confirmed defects from candidate issues when the evidence level differs.
- If the input includes prior review notes, broad branch audit notes, or another AI review, mark items that are already fixed or superseded instead of restating them as current defects.

### Open Questions

- Call out assumptions that could change the review outcome.

### Verification Gaps

- State missing tests, missing commands, missing reviewer coverage, intentionally unreviewed scope, or missing smoke checks.

### Summary

- Keep this short.

If there are no findings:

- say that explicitly
- mention residual risks or missing verification

## 10. Self-Review Before Completion

Before claiming a change is done, the author should ask:

- Does the change match the approved intent?
- Am I reviewing and judging this change against the documented design standard and implementation plan?
- Is this change too large or too mixed-scope for a reliable review, and if I did not split it, did I explain why?
- What can regress?
- What still lacks proof?
- What boundary changed?
- Is code review being asked to settle a design question that should already be documented in the change docs?
- What qualified reviewer or domain expertise does this change need?
- Am I anchoring only on the currently reported bug cluster, active change narrative, or the files already named in the review prompt?
- What shared infrastructure or foundational code could still invalidate this change even if the edited files look correct?
- What happens on the second call, retry, reconnect, or replacement of the same resource?
- If an object reference or handle is overwritten, what explicitly closes and unregisters the previous runtime object?
- What failure path did I inspect explicitly instead of assuming safe?
- Does the declared wire contract exactly match what the runtime emits and what the counterpart expects?
- If I parsed or generated protocol fields, did I verify signedness, units, timebase, and timestamp semantics against the spec and counterpart implementation?
- If buffers, builders, offsets, pointers, views, or callback captures cross scopes, resets, or threads, did I verify their lifetime rules explicitly?
- What thread boundary, callback path, timer path, or teardown race did I inspect explicitly?
- If a metric can come from different sources, are baseline, reset, and fallback transitions safe?
- For high-risk C++ runtime paths, what sanitizer-like, crash-recovery, stress, or performance evidence is applicable here?
- Did I reuse existing code where practical and reduce duplication instead of adding another parallel path?
- Did I keep the change as simple and self-contained as practical instead of mixing in avoidable churn?
- Did I avoid speculative abstractions and keep names, comments, and error handling explicit?
- What production failure mode have I not considered?
- Am I letting test count, report size, or a large test suite substitute for verifying the risky scenarios that actually changed?
- What documentation became stale or less precise because of this change, and has it been updated before commit?
- If this branch is compared against another branch, what global behavior change does the branch introduce?
- Which findings are proven, which are only suspected, and what evidence separates them?

If those answers are unclear, the work is not ready to close.
