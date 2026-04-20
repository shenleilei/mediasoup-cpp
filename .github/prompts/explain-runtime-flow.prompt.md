# Explain Runtime Flow

Use this prompt when you need a concrete, file-backed explanation of how a runtime path works.

Read first:

- the relevant implementation files
- the nearest accepted spec in `specs/current/`
- the nearest architecture or operational doc in `docs/`, if one exists
- the specific scenario to trace

Task:

1. Explain the execution flow in order from entrypoint to teardown.
2. Name the major functions, classes, and hand-off points involved.
3. Call out invariants, lifecycle rules, and failure handling.
4. If the path is stateful or protocol-heavy, include repeated-call, reconnect, replacement, and cleanup behavior where relevant.

Output format:

1. Scenario being traced
2. Entry point
3. Step-by-step flow with file references
4. State transitions or invariants
5. Failure handling and cleanup
6. Open questions or unclear areas

Constraints:

- Use concrete file references, not generic summaries.
- Distinguish current behavior from inference.
- Do not speculate about accepted behavior when the spec says otherwise.
