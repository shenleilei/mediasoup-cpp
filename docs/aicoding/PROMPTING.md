# Prompting Guide

This document is a supplement to the repository's shared AI workflow docs.

It does not replace:

- `PROJECT_STANDARD.md`
- `PLANS.md`
- `REVIEW.md`
- active change docs under `changes/`
- accepted behavior in `specs/current/`

Use this file to improve prompt quality and IDE workflow ergonomics. Do not use it to weaken delivery gates, invent requirements, or hide project memory in chat-only instructions.

## Priority And Scope

For AI-assisted work in this repository:

1. `docs/aicoding/PROJECT_STANDARD.md` always wins.
2. Use the active change folder under `changes/` to define the current unit of work.
3. Use `docs/aicoding/PLANS.md` when a plan is required and `docs/aicoding/REVIEW.md` for review and self-review behavior.
4. Use `specs/current/` for accepted product or system behavior.
5. Treat `AGENTS.md`, `CLAUDE.md`, `.github/copilot-instructions.md`, and `.github/instructions/*.instructions.md` as tool-facing helpers that must stay aligned with the shared docs.
6. Treat `.github/prompts/*.prompt.md` as optional task starters.
7. Treat chat-local wording for the current request as the most disposable layer.

Prompt files and instruction files are helpers. They are not durable project memory and they do not override the workflow standards.

## What These Prompting Assets Are For

Use prompt assets to:

- package the right repo context for a common task
- ask for a predictable output structure
- reduce repeated prompt writing in IDE workflows
- improve consistency across Codex, Copilot, and similar tools

Do not use prompt assets to:

- create a second source of truth
- store requirements that never land in repo files
- skip change docs for non-trivial work
- skip tests, review, or delivery checks
- rely on role-play instead of explicit technical constraints

## Prompt Shape

Good repo prompts usually contain five parts:

1. Task: what the tool should do now
2. Scope: what is in scope and what is not
3. Context: which repo docs and code areas to read first
4. Constraints: workflow, compatibility, review, and verification rules
5. Output: the shape of the answer, diff, review, or test plan you want back

Prefer direct instructions over persona-heavy prompts.

Prefer explicit file references over vague descriptions like "the backend code" or "the latest docs".

Prefer asking for verification mapping over broad requests like "make it production ready".

## Context Packaging By Task

### Structured Change

Attach or point to:

- `docs/aicoding/PROJECT_STANDARD.md`
- `docs/aicoding/PLANS.md`
- `docs/aicoding/REVIEW.md`
- the relevant accepted spec under `specs/current/`
- the active change folder, or ask the tool to create one

Ask the tool to:

- classify the work as Quick Task or Structured Change
- create or update `requirements.md` or `bugfix.md`, `design.md`, and `tasks.md` before major edits
- map acceptance criteria to verification
- keep exactly one plan step in progress

### Bugfix Or Regression Work

Attach or point to:

- the failing test, log, or reproduction command
- the active bugfix change docs
- the relevant runtime code and existing tests

Ask the tool to:

- state the observed behavior and expected behavior separately
- add or update regression coverage
- review repeated call, reconnect, replacement, and cleanup paths when relevant

### Whole-System Review

Attach or point to:

- `docs/aicoding/REVIEW.md`
- the full branch diff or changed file set
- active change docs
- affected specs and operational docs

Ask the tool to:

- review from the overall system perspective
- list findings before summary
- call out missing tests, contract drift, lifecycle risks, and documentation drift

### Runtime Explanation

Attach or point to:

- the relevant source files
- the accepted spec or architecture doc if one exists
- the specific scenario you want traced

Ask the tool to:

- explain the end-to-end flow in execution order
- identify invariants, state transitions, and failure handling
- cite concrete file paths for each step

## Output Shaping

Ask for structure when the output will be consumed by a human or another tool.

Useful output shapes include:

- a numbered execution plan with explicit verification
- a requirements list with SHALL-style statements
- a review list ordered by severity with file references
- a test matrix covering success, edge, and failure paths
- a runtime walk-through with entrypoint, transitions, cleanup, and failure cases

When parsing or follow-up automation matters, prefer explicit sections over free-form prose.

## Prompt Iteration

When a prompt underperforms, do not patch it blindly.

Use a small failure-driven loop:

1. collect 2 to 5 representative misses
2. identify the repeated failure mode
3. tighten the prompt with a missing constraint, context file, or output shape
4. rerun on the same cases
5. keep the change only if it improves the failure set without weakening the good cases

Prefer improving repo prompt assets over repeating long ad hoc chat instructions.

## Few-Shot And Examples

Examples help when the repository expects a specific structure that the tool repeatedly misses.

Use examples sparingly:

- one compact example is usually enough
- prefer repo-native examples over generic internet examples
- never let examples contradict current repo standards

If the tool already follows the required structure, avoid adding examples just for style.

## Structured Outputs

When you need machine-consumable or review-friendly output, ask for structure explicitly.

Examples:

- "Return the plan in the `PLANS.md` template shape."
- "Return review findings first, each with severity, impact, and file reference."
- "Map each acceptance criterion to a verification command or check."

For repo work, structured output is usually more useful than creative prose.

## What Not To Encode In Prompts

Do not encode:

- secret values or credentials
- requirements that belong in `changes/` or `specs/current/`
- instructions that conflict with `PROJECT_STANDARD.md`
- hidden assumptions about skipping tests or review
- broad "be an expert architect" style role text without task-specific constraints

## GitHub Customization Surfaces

This repository uses three GitHub-facing layers:

- `.github/copilot-instructions.md`: thin repo-wide entrypoint
- `.github/instructions/*.instructions.md`: path-specific guidance
- `.github/prompts/*.prompt.md`: reusable task starters

Keep each layer narrow:

- standards stay in `docs/aicoding/`
- path-specific instructions stay local to the area they affect
- reusable prompts stay task-oriented and optional

## External References

These references informed the approach behind this supplement:

- GitHub Copilot repository, path-specific instruction, and prompt-file docs:
  - `https://docs.github.com/en/copilot/how-tos/configure-custom-instructions/add-repository-instructions`
  - `https://docs.github.com/en/copilot/customizing-copilot/adding-custom-instructions-for-github-copilot`
- OpenAI Cookbook and developer resources on structured outputs and prompt refinement:
  - `https://developers.openai.com/cookbook/examples/structured_outputs_intro`
  - `https://developers.openai.com/resources/`
- selected community prompt-engineering references used as optional heuristics, not as repo standards
