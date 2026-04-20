# Write A Regression Test

Use this prompt when a change needs targeted regression coverage.

Read first:

- the active change docs under `changes/`
- the affected implementation files
- existing tests near the touched behavior
- [`docs/aicoding/PROJECT_STANDARD.md`](../../docs/aicoding/PROJECT_STANDARD.md)
- [`docs/aicoding/REVIEW.md`](../../docs/aicoding/REVIEW.md)

Task:

1. Identify the precise failure mode or changed behavior.
2. Add or update targeted tests close to the affected logic.
3. Cover normal path, edge case, and failure path for the changed business logic.
4. If the behavior is stateful, also cover repeated call, reconnect, replacement, and cleanup paths when applicable.
5. Keep the test names and assertions explicit about the protected behavior.

Output format:

1. Failure mode being protected
2. Test file and case names to add or update
3. Why those tests cover the regression risk
4. Verification commands

Constraints:

- Do not weaken or delete coverage just to make the change pass unless the spec changed.
- Prefer deterministic setup over sleeps or broad timing assumptions.
- Update accepted specs or docs if the behavior contract changed.
