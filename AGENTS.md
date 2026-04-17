# Agent Instructions

Primary AI-coding docs live under `docs/aicoding/`.

Read before planning or coding:

- `docs/aicoding/PROJECT_STANDARD.md`
- `docs/aicoding/PLANS.md`
- `docs/aicoding/REVIEW.md`

## Priority

- `docs/aicoding/PROJECT_STANDARD.md` is the source of truth
- `docs/aicoding/PLANS.md` defines how plans are created and maintained
- `docs/aicoding/REVIEW.md` defines how changes are reviewed
- The active change folder under `changes/` defines the current unit of work
- `specs/current/` defines accepted behavior

## Workflow

- Use a Structured Change for non-trivial work
- Do not code before requirements or bug intent is clear
- Keep edits narrow and traceable to tasks
- Run unit tests, integration tests, and required checks before claiming completion
- Use `docs/aicoding/DELIVERY_CHECKLIST.md` before merge or handoff
- Keep tool-specific instruction files aligned with `docs/aicoding/`

## Guardrails

- Do not invent requirements that are not written down
- Do not hide unresolved scope inside implementation
- Do not treat chat history as durable project memory
- Do not leave tool-specific instruction files in conflict
- Do not claim "done" based only on code changes without verification evidence
