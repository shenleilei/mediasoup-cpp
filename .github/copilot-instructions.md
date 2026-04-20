# Copilot Repository Instructions

Primary AI-coding docs live under `docs/aicoding/`.

- `docs/aicoding/PROJECT_STANDARD.md` is the source of truth for project workflow and quality gates.
- `docs/aicoding/REVIEW.md` defines how review and self-review should be performed.
- `docs/aicoding/PLANS.md` defines how to create and maintain execution plans for non-trivial work.
- `docs/aicoding/PROMPTING.md` defines supplemental prompting and context-packaging guidance.
- `AGENTS.md` contains the repository entrypoint for agent instructions.
- For non-trivial work, use a change folder under `changes/` with requirements or bugfix, design, and tasks.
- Path-specific guidance may live under `.github/instructions/`.
- Reusable prompt starters may live under `.github/prompts/`.
- Do not treat code generation as completion; required tests and delivery gates still apply.
- Keep instructions here thin to avoid conflicts with other instruction files.
- Prompt files and path-specific instructions supplement the workflow docs; they do not override them.
