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
- Treat non-trivial refactors and modernization work as Structured Changes and follow the Refactoring Standard in `docs/aicoding/PROJECT_STANDARD.md`
- Do not code before requirements or bug intent is clear
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
- Run unit tests, integration tests, and required checks before claiming completion
- Use `docs/aicoding/DELIVERY_CHECKLIST.md` before merge or handoff
- Keep tool-specific instruction files aligned with `docs/aicoding/`

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
