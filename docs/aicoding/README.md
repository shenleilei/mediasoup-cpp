# AI Coding Docs

This directory holds the repository's shared AI-coding workflow docs.

## Fixed Entry Points

Some tools still expect fixed file locations. Those entry points remain at:

- `AGENTS.md`
- `CLAUDE.md`
- `.github/copilot-instructions.md`

They are thin wrappers that point back to this directory.

## Core Docs

- `PROJECT_STANDARD.md`: primary workflow and delivery standard
- `PLANS.md`: execution-planning rules
- `REVIEW.md`: review and self-review standard
- `DELIVERY_CHECKLIST.md`: pre-merge delivery gate
- `AGENTS.md`: full shared agent instruction body
- `CLAUDE.md`: Claude-specific entry note
- `research-notes.md`: background on why this structure exists

## Adjacent Workflow Directories

These stay at their conventional paths because the workflow depends on them:

- `changes/`
- `specs/current/`

`changes/_templates/` and `specs/current/README.md` remain in place for discoverability at point of use.

Active change folders may also carry an optional `implementation.md` when execution notes help keep progress and deferred work truthful during a long-running change. The required planning artifacts remain `requirements.md` or `bugfix.md`, `design.md`, and `tasks.md`.
