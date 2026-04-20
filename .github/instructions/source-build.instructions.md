---
applyTo: "src/**/*,client/**/*,common/**/*,cmake/**/*,CMakeLists.txt,client/CMakeLists.txt,scripts/**/*.py"
---

For source, build, and automation files in this repository:

- Treat non-trivial behavior changes as Structured Changes and keep them tied to one active change folder under `changes/`.
- Prefer existing helpers, abstractions, and proven code paths over parallel implementations.
- Keep diffs narrow and separate large refactors, renames, and formatting churn from behavior changes when practical.
- Be explicit about lifecycle, ownership, teardown, retries, reconnects, and replacement behavior for stateful or protocol-heavy code.
- Preserve predictable naming across code, tests, docs, and configs.
- Add comments only when they explain constraints, invariants, or tradeoffs.
- Update or add tests whenever business logic or workflow behavior changes.
