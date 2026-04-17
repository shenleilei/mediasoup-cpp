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
- Use `PLANS.md` when a plan is required
- Do not code before requirements or bug intent is clear
- Do one task at a time
- Keep edits narrow and traceable to tasks
- Run unit tests, integration tests, and required checks before claiming completion
- Use `REVIEW.md` for review work and pre-completion self-review
- Review from the whole-system perspective, not only the edited file perspective
- Update docs when accepted behavior changes

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
- do not treat the visible diff hunk as the full scope of review
