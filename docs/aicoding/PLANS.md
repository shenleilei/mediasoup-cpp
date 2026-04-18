# Execution Plans

This file defines how coding agents should create, maintain, and execute plans for non-trivial work.

`PLANS.md` governs execution behavior.

`PROJECT_STANDARD.md` governs delivery standards.

`changes/` documents govern the content of the current change.

If these files conflict, follow this order:

1. `PROJECT_STANDARD.md`
2. Active change documents under `changes/`
3. `REVIEW.md`
4. `PLANS.md`
5. Tool-specific instruction files

## 1. Purpose

Use plans to keep complex work controlled, reviewable, and verifiable.

A plan is required to:

- break work into clear steps
- make verification explicit before coding starts
- prevent scope drift
- surface blockers early
- keep updates honest while the work is in progress

Plans are internal execution artifacts, not product specs or user approval checkpoints.

Do not use a plan as a substitute for `requirements.md`, `bugfix.md`, `design.md`, or `tasks.md`.

## 2. When A Plan Is Required

Create and maintain a plan when any of the following is true:

- the work is a Structured Change
- more than one module, service, or boundary is affected
- the change touches APIs, events, schemas, queues, caches, auth, or workflows
- the task needs more than one meaningful implementation step
- the task requires coordination between code changes and tests
- the task has non-trivial risk, rollback, or migration concerns
- the user explicitly asks for a plan

## 3. When A Plan Is Optional

A formal plan can be skipped only for true Quick Tasks:

- tightly local code edit
- no contract or schema changes
- low regression risk
- behavior is already obvious
- verification is small and direct

If the task expands during execution, immediately upgrade to a plan.

## 4. Relationship To Change Documents

For Structured Change work, the normal order is:

1. define intent in `requirements.md` or `bugfix.md`
2. define approach in `design.md`
3. define executable units in `tasks.md`
4. create or update the execution plan
5. implement and verify

The plan should reference the active change folder.

The plan must not introduce new requirements that are absent from the change docs.

## 5. Plan Format

Every plan should contain the following sections.

### Change Context

- change type: Quick Task or Structured Change
- active change folder, if any
- short objective

### Scope

- in-scope work
- explicit non-goals

### Constraints

- architectural, product, operational, or delivery constraints
- test obligations
- compatibility or rollout constraints

### Steps

- 3 to 7 concrete steps
- exactly one step marked `in_progress`
- every step must have a clear outcome

### Verification

- commands, suites, or checks that will prove the change is done
- unit test expectations
- integration or contract test expectations
- manual smoke checks only if they supplement automation

### Risks And Blockers

- known risks
- unresolved questions
- dependencies that may block execution

## 6. Step Quality Rules

Good plan steps are:

- outcome-oriented
- independently understandable
- small enough to finish without hidden subprojects
- mapped to code areas and verification

Bad plan steps include:

- "finish backend"
- "make tests pass"
- "refactor everything"
- "handle remaining issues"

## 7. Execution Rules

When following a plan:

- do not start major edits before the plan exists if a plan is required
- work one step at a time
- keep exactly one step `in_progress`
- update the plan when scope or sequencing changes
- if a blocker invalidates the plan, revise the plan before continuing
- if a Quick Task becomes risky or cross-boundary, upgrade it to Structured Change
- do not mark implementation complete until verification steps are executed

## 8. Verification Rules

Every plan must reflect the delivery standard.

At minimum, include applicable checks for:

- unit tests for changed business logic
- integration tests for cross-boundary behavior
- regression tests for bugfixes
- lint, type-check, build, or static analysis
- contract validation for API, schema, event, permission, or state changes
- `DELIVERY_CHECKLIST.md` review before completion

If no automated verification exists where it should, record that as delivery debt explicitly.

## 9. Communication Rules

Before substantial work:

- state whether a plan is required
- if required, summarize the execution plan before major edits
- do not wait for plan approval unless the user explicitly asks to review or approve the plan first

During execution:

- report completed steps and the current in-progress step
- call out new risks or scope changes immediately
- continue across step boundaries without turning each step into a user handoff unless blocked

At completion:

- summarize what changed
- summarize what verification ran
- summarize the review result or remaining review debt
- state any remaining debt, waivers, or follow-up items

## 10. Default Plan Template

Use this template when writing or updating a plan in chat or in a change document.

```md
## Execution Plan

### Change Context
- Type:
- Change folder:
- Objective:

### Scope
- In scope:
- Out of scope:

### Constraints
- Constraints:
- Required tests:
- Compatibility or rollout notes:

### Steps
1. [completed|in_progress|pending] Step name
   Outcome:
   Verification:
2. [completed|in_progress|pending] Step name
   Outcome:
   Verification:
3. [completed|in_progress|pending] Step name
   Outcome:
   Verification:

### Risks And Blockers
- Risk:
- Blocker:
```

## 11. Defaults For Codex

Unless the user explicitly asks otherwise, Codex should:

- create a plan for Structured Change work
- keep the plan concise
- prefer execution over prolonged planning
- revise the plan rather than silently drifting
- use the plan to drive implementation and verification updates
- continue from planning to implementation, review, refactor, and verification without waiting for intermediate approval when intent is clear
- ask for user input only when requirements are missing or conflicting, a destructive or irreversible action is required, external access or secrets are required, or the user explicitly requests intermediate approval
