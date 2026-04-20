# Plan A Structured Change

Use this prompt for non-trivial feature, refactor, or bugfix work in this repository.

Read first:

- [`docs/aicoding/PROJECT_STANDARD.md`](../../docs/aicoding/PROJECT_STANDARD.md)
- [`docs/aicoding/PLANS.md`](../../docs/aicoding/PLANS.md)
- [`docs/aicoding/REVIEW.md`](../../docs/aicoding/REVIEW.md)
- the relevant file under `specs/current/`
- the active change folder under `changes/`, or create one if the work is non-trivial and no change folder exists yet

Task:

1. Classify the work as Quick Task or Structured Change.
2. If it is a Structured Change, create or update `requirements.md` or `bugfix.md`, `design.md`, and `tasks.md` before major edits.
3. Produce an execution plan that follows `PLANS.md` and has exactly one `in_progress` step.
4. Map acceptance criteria to verification before implementation starts.
5. Keep the implementation narrow and traceable to the task list.

Output format:

1. Classification
2. Active change folder
3. Key requirements or bug intent
4. Execution plan
5. Immediate next implementation step
6. Verification plan

Constraints:

- Do not invent requirements that are not written down.
- Do not skip tests, review, or delivery gates.
- Prefer repo files over chat history as durable memory.
- If scope is unclear, resolve that in the change docs before coding.
