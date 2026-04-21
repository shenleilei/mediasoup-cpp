# Agent Instructions

Read `PROJECT_STANDARD.md` before planning or coding.
Read `PLANS.md` before non-trivial execution.
Read `REVIEW.md` before code review or self-review.

## Priority

- `PROJECT_STANDARD.md` is the source of truth
- `PLANS.md` defines how plans are created and maintained
- `REVIEW.md` defines how changes are reviewed
- The active change folder under `changes/` defines the current unit of work
- `specs/current/` defines accepted behavior

## Workflow

- Use a Structured Change for non-trivial work
- Treat non-trivial refactors and modernization work as Structured Changes and follow the Refactoring Standard in `PROJECT_STANDARD.md`
- Use `PLANS.md` when a plan is required
- Do not code before requirements or bug intent is clear
- Do one task at a time
- Keep edits narrow and traceable to tasks
- Prefer reusing existing code, helpers, abstractions, and proven paths before adding new parallel implementations
- When the work exposes duplication, brittle structure, or repeated patterns, make small safe refactors as part of the change instead of layering more duplication
- Prefer small, self-contained changes; separate large refactors, renames, and formatting sweeps from behavior changes when practical
- Prefer simple, explicit implementations over cleverness, hidden control flow, or speculative extensibility
- Solve the concrete requirement that exists now; avoid over-engineering for hypothetical future needs
- Keep naming predictable and consistent across code, tests, configs, and docs; one concept should usually have one name
- Handle errors, edge cases, and fallback paths explicitly; do not silently swallow failures without a documented reason
- Keep comments focused on why, constraints, invariants, and tradeoffs; if a comment only explains what the code says, simplify the code instead
- Minimize new dependencies, indirection, and hidden coupling; add a new abstraction or library only when existing code cannot reasonably cover the need
- When downloading third-party build dependencies or toolchain prerequisites, prefer Alibaba Cloud (`Aliyun`) mirrors first and only fall back to upstream when the mirror is missing the required artifact
- Run unit tests, integration tests, and required checks before claiming completion
- Use `REVIEW.md` for review work and pre-completion self-review
- Review from the whole-system perspective, not only the edited file perspective
- Update docs when accepted behavior changes

## Autonomy Default

- For coding, review, and refactor work, continue from planning through implementation, self-review, refactor, and verification in one run when intent is clear
- Treat plans, task lists, checklists, and self-review as internal execution controls, not user approval gates
- Do not pause for user confirmation between internal milestones unless requirements are missing or conflicting, a destructive or irreversible action is required, external access or secrets are required, or the user explicitly asks to approve intermediate steps
- Prefer concise progress updates during execution and hand off at the end with the verified result, remaining risks, and follow-up items

## Guardrails

- Do not invent requirements that are not written down
- Do not hide unresolved scope inside implementation
- Do not treat chat history as durable project memory
- Do not leave tool-specific instruction files in conflict
- Do not claim "done" based only on code changes without verification evidence

## Expected Inputs Before Coding Complex Work

- Relevant accepted specs in `specs/current/`
- One active change folder in `changes/`
- Execution sequencing via `PLANS.md`
- Clear verification plan
- Review criteria via `REVIEW.md`
- Delivery gate review via `DELIVERY_CHECKLIST.md`

## Review Guardrail

When comparing branches or reviewing a pull request:

- review the entire change set against the base branch
- consider contracts, tests, docs, configs, migrations, and operational impact
- protect overall code health: simplicity, consistency, explicit error handling, and maintainability matter more than personal style preferences
- treat avoidable duplication, missed reuse, and skipped low-risk refactors as review concerns when they increase maintenance cost or inconsistency risk
- do not treat the visible diff hunk as the full scope of review
