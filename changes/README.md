# Changes Workflow

This directory holds proposed and in-flight work.

## Naming

Use:

- `changes/YYYY-MM-DD-short-name`

Example:

- `changes/2026-04-17-add-order-cancel-reason`

## Structure

Feature change:

```text
changes/2026-04-17-add-order-cancel-reason/
|-- requirements.md
|-- design.md
`-- tasks.md
```

Bugfix change:

```text
changes/2026-04-17-fix-login-token-refresh/
|-- bugfix.md
|-- design.md
`-- tasks.md
```

## Lifecycle

1. Create the folder.
2. Draft requirements or bugfix analysis.
3. Review until intent is clear.
4. Write design.
5. Break into tasks.
6. Implement and verify.
7. Merge accepted behavior into `specs/current/`.
8. Archive or remove the change folder according to team practice.
