---
applyTo: "tests/**/*,specs/current/**/*"
---

For test files and accepted specs:

- Map changed behavior to explicit success, edge, and failure coverage.
- For stateful resources or workflows, cover repeated call, retry, reconnect, replacement, and cleanup behavior when applicable.
- Prefer targeted assertions that explain the protected behavior over broad smoke-only assertions.
- Do not silently bypass, weaken, or orphan existing coverage when tests move or split.
- Keep accepted specs aligned with the final documented behavior when behavior contracts change.
- If automated coverage is missing where it should exist, record that gap explicitly instead of treating it as done.
