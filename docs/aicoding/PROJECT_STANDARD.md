# Project Development Standard

This file is the primary source of truth for how this project is planned, implemented, verified, reviewed, and documented.

If any agent instruction file conflicts with this document, this document wins.

## 1. Working Principles

- Intent before implementation. For non-trivial work, agree on behavior before writing code.
- One source of truth. Stable project rules live here, not in chat history.
- Small tasks. Every implementation task should be independently understandable and verifiable.
- Tests are completion gates. A task is not done because code exists; it is done when verification passes.
- Docs follow reality. Accepted behavior must be reflected in `specs/current/`.
- Documentation stays current. Before every commit, update all affected docs so architecture notes, runbooks, specs, examples, and workflow guidance still match the code.
- Minimize instruction drift. Tool-specific instruction files must be thin wrappers around this standard.

## 2. Work Sizing

### Quick Task

Use a direct coding flow without a formal change folder only when all conditions hold:

- Single module or tightly local edit
- No schema or API contract changes
- No state machine or workflow changes
- Low regression risk
- Expected behavior is already obvious

Examples:

- Rename a variable
- Add a missing null check
- Adjust a small UI label or layout issue
- Add a targeted unit test for existing behavior

### Structured Change

Use a change folder under `changes/` when any of the following is true:

- Cross-module or cross-service impact
- New user-visible behavior
- Bugfix with unclear root cause or regression risk
- API, schema, permission, or configuration contract change
- Performance, reliability, or security work
- Work needs review before coding

Required artifacts:

- Feature work: `requirements.md`, `design.md`, `tasks.md`
- Bugfix work: `bugfix.md`, `design.md`, `tasks.md`

## 3. Non-Negotiable Delivery Constraints

The following rules apply to all behavior-changing work unless explicitly waived.

### Test Obligations

- New or changed business logic MUST have unit tests.
- Unit tests MUST cover normal path, edge cases, and failure path for the changed logic.
- Cross-module, cross-service, API, database, queue, cache, or workflow changes MUST have integration tests.
- Bugfixes MUST include regression coverage for the original failure mode.
- Critical user journeys SHOULD have smoke or end-to-end coverage if the project supports it.

### Verification Obligations

- If the repository has lint, type-check, build, or static analysis steps, they MUST pass before the work is called complete.
- Acceptance criteria are not complete until they are mapped to executable checks or explicit manual verification.
- If a change modifies contracts, schemas, or state transitions, the verification plan MUST include those boundaries directly.

### Delivery Obligations

- Do not mark a task done because code compiles or the agent says implementation is finished.
- Do not merge behavior-changing work without tests unless the debt is explicitly accepted.
- Do not remove or weaken tests to make a change pass unless the spec itself changed and the impact is documented.
- For risky changes, include rollback, mitigation, or feature-flag strategy.
- For operationally important flows, add enough logs, metrics, or traces to diagnose failures in production.
- Before every commit, review and update all affected documentation, not only code-adjacent files.
- Do not leave stale or partially incorrect documentation behind after behavior, assumptions, control flow, or operational procedures change.

### External Dependency Sources

- When downloading third-party build dependencies, source archives, language packages, or toolchain prerequisites, prefer Alibaba Cloud (`Aliyun`) mirror infrastructure first when it provides the required artifact.
- If the Aliyun mirror does not contain the required artifact, version, or patch file, fall back to the upstream source and record that fallback in the active change docs or handoff.
- When build scripts or wrap files repeatedly fetch upstream dependencies, update the local configuration to prefer the Aliyun mirror before relying on ad hoc command-line overrides.

### Review Obligations

- Requirements review asks: are we building the right thing?
- Design review asks: is the chosen approach safe, scalable, and understandable?
- Code review asks: does the implementation actually match the spec and preserve invariants?
- Verification review asks: do the tests and checks prove the intended behavior?

## 4. Standard Workflow

### Step 1: Classify The Work

Decide whether the request is a Quick Task or Structured Change.

When uncertain, choose Structured Change.

### Step 2: Check Project Memory

Read:

- `PROJECT_STANDARD.md`
- `PLANS.md`
- `REVIEW.md`
- `AGENTS.md`
- Relevant files in `.kiro/steering/`
- Relevant accepted specs in `specs/current/`

Update these files when project conventions change.

### Step 3: Define The Change

For features, write `requirements.md`.

For bugs, write `bugfix.md`.

Do not start implementation until the intended behavior is clear enough to review.

### Step 4: Design The Solution

Write `design.md` for any change that affects architecture, data, flow, interfaces, or failure handling.

The design should explain why the chosen approach is correct, not just what files will change.

### Step 5: Decompose Into Tasks

Write `tasks.md` as a sequence of small tasks.

Each task should state:

- Outcome
- Main files or modules touched
- Verification command or check

Good tasks can be completed and reviewed independently.

### Step 6: Create Or Update The Execution Plan

If the work is non-trivial, create or update the execution plan according to `PLANS.md`.

The execution plan should sequence the work, define what is currently in progress, and make verification explicit.

### Step 7: Implement

Implementation rules:

- Work task by task
- Keep diffs narrow
- Avoid unrelated refactors inside feature work
- Preserve backward compatibility unless the spec explicitly changes it
- Add logging or metrics when operational debugging will matter
- Prefer reversible migrations and rollout steps when risk exists

### Step 8: Verify

Minimum verification for behavior-changing work:

- Lint or formatting checks if the repo uses them
- Type checks or build checks if the repo uses them
- Unit tests for all changed business logic
- Integration tests when module boundaries are crossed
- Regression tests for fixed bugs
- Contract verification for API, schema, event, permission, or state-machine changes
- Manual smoke validation only as a supplement, not as a replacement for automated checks

For rules-heavy or algorithmic behavior, prefer invariant- or property-style testing in addition to example-based tests.

Before declaring completion, run through `DELIVERY_CHECKLIST.md`.

### Step 9: Land And Update Knowledge

Before a change is considered complete:

- Run through `REVIEW.md`
- Update accepted behavior in `specs/current/` if the change is approved
- Update all affected documentation so descriptions, examples, and operational notes remain accurate
- Mark tasks complete with real status
- Record any rollout notes or follow-up items
- Remove obsolete instructions that would mislead future agents

## 5. Artifact Standards

### Requirements

`requirements.md` should contain:

- Problem statement
- Business goal
- In scope
- Out of scope
- User stories or scenarios
- Acceptance criteria
- Non-functional requirements
- Edge cases and failure cases
- Open questions

Rules:

- Describe behavior, not implementation
- Use clear SHALL or MUST style statements when precision matters
- Include non-goals to control scope
- State any explicit performance, security, reliability, or compatibility constraints

### Bugfix

`bugfix.md` should contain:

- Symptom
- Reproduction
- Observed behavior
- Expected behavior
- Suspected scope
- Known non-affected behavior
- Acceptance criteria
- Regression expectations

Rules:

- Capture what must not regress
- Prefer adding the failing test before the fix when practical

### Design

`design.md` should contain:

- Context
- Proposed approach
- Alternatives considered
- Module boundaries
- Data structures or schema changes
- API or interface changes
- Control flow or state transitions
- Failure handling
- Security considerations
- Observability
- Testing strategy
- Rollout or migration notes

Rules:

- Show decision rationale
- Call out tradeoffs
- Make assumptions explicit
- State what will prove correctness before implementation starts

### Tasks

`tasks.md` should contain only executable work.

Rules:

- Each task should be independently testable
- Prefer tasks that can be completed in hours, not days
- Include validation per task
- Avoid vague tasks such as "finish backend"
- Separate implementation from verification work when that improves clarity
- Include unit test and integration test tasks where applicable

## 6. Verification Matrix

Use this matrix as the project default.

### Pure Local Logic

Required:

- Unit tests
- Lint, type-check, build if applicable

### Module Or Service Boundary Change

Required:

- Unit tests
- Integration tests across the boundary
- Contract verification for request and response behavior

### Data Or Schema Change

Required:

- Unit tests for transformation logic
- Integration tests against persistence behavior
- Migration verification
- Rollback or backward compatibility review

### Bugfix

Required:

- Reproduction test or reproducible check
- Fix validation
- Regression test
- Nearby-path validation

### Performance Or Reliability Change

Required:

- Unit tests where logic changes
- Integration or workload validation
- Measured before-and-after evidence

### Security-Sensitive Change

Required:

- Unit tests for policy logic
- Integration tests for protected paths
- Review of auth, authz, secrets, and sensitive data handling

## 7. Quality Gates

Definition of done for a non-trivial change:

- Behavior is specified
- Design is documented when complexity justifies it
- Tasks are completed honestly
- Code matches the approved behavior
- Unit tests and required integration tests have run and results are known
- Required CI-quality checks have passed or are explicitly waived
- Review has been performed according to `REVIEW.md`
- The review considered global system impact, not just local file edits
- Accepted specs are updated
- Operational and migration concerns are documented if relevant

## 8. Knowledge Governance

Use the following split:

- `PROJECT_STANDARD.md`: project-wide source of truth
- `PLANS.md`: execution planning protocol
- `REVIEW.md`: review protocol
- `AGENTS.md`: cross-agent operational instructions
- `CLAUDE.md`: Claude-specific entrypoint that should defer to shared docs
- `.github/copilot-instructions.md`: Copilot-specific thin wrapper
- `.kiro/steering/`: Kiro-specific persistent context files
- `specs/current/`: accepted product and system behavior
- `changes/`: proposed or in-flight work

Rules for governance:

- Do not duplicate large blocks of instructions across files
- Keep tool-specific files short and non-conflicting
- Update the nearest relevant scope, not every file blindly
- When a decision becomes stable, move it from chat or PR discussion into project memory

## 9. Naming And Lifecycle

Change folder naming:

- `changes/YYYY-MM-DD-short-name`

Lifecycle:

1. Draft
2. Review
3. Implement
4. Verify
5. Merge accepted behavior into `specs/current/`
6. Archive or remove the change folder according to team practice

## 10. Defaults For Small Teams

If the team has not defined stricter rules yet, use these defaults:

- One branch per change folder
- One owner per change
- One reviewer for low-risk changes, two for risky changes
- Keep feature flags for risky rollout paths
- Never merge behavior-changing work without tests unless explicitly accepted as debt
- New or changed business logic should not land without unit tests
- Boundary-changing work should not land without integration tests

## 11. Refactoring Standard

Refactoring and code modernization work follows these defaults in addition to the rest of this standard.

### Default Intent

- Default to behavior-equivalent changes unless accepted specs explicitly require behavior changes.
- Treat non-trivial refactors and modernization work as Structured Changes.
- Optimize for clarity, maintainability, testability, and consistency.
- Preserve public contracts, data formats, and operational semantics unless the spec changes them.
- Reuse existing helpers and abstractions before creating new ones.
- Do not introduce broad exception handling, silent fallbacks, hidden retries, or fake-success returns.
- Do not overwrite unrelated changes while refactoring.

### Required Inputs

Before starting a non-trivial refactor, capture these in the active change folder or task brief:

- Goal
- Context
- Constraints
- Done when

Use the existing project artifacts:

- `requirements.md` or `bugfix.md` defines why the refactor exists and what behavior must stay stable.
- `design.md` records module boundaries, invariants, migration order, and parity strategy.
- `tasks.md` breaks the refactor into independently verifiable steps.
- `PLANS.md` governs sequencing and execution tracking for larger refactors.

### Refactoring Workflow

- Inventory current call paths, data flow, dependencies, and verification coverage before editing.
- Make behavior-preservation assumptions explicit in `design.md`.
- Split large refactors into small, rollback-friendly batches.
- Separate structural moves, abstraction extraction, and intentional behavior changes when practical.
- Compare old and new behavior with parity checks, regression tests, or fixture-based validation.
- Review the final diff for accidental behavioral drift, deleted edge-case handling, and scope creep.

### Code And Verification Rules

- Add or update tests when the refactor touches behavior boundaries, bug-prone logic, or public contracts.
- Run relevant lint, type-check, build, unit, and integration checks before completion.
- Cover happy paths, edge cases, and failure paths that define current behavior.
- Prefer narrow, traceable diffs; avoid mixing opportunistic cleanup into risky refactors.
- If behavior changes intentionally, document the delta and update `specs/current/`.

### Completion Criteria

- The refactor is backed by explicit verification evidence.
- Behavior matches prior accepted behavior or documented changes.
- The resulting structure is simpler to understand, test, and maintain.
- Remaining risks, deferred cleanup, and follow-up work are recorded honestly.
