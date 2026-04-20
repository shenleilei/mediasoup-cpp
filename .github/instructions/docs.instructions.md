---
applyTo: "docs/**/*,AGENTS.md,CLAUDE.md,.github/copilot-instructions.md,.github/instructions/**/*,.github/prompts/**/*"
---

For docs and AI instruction files:

- Keep `docs/aicoding/PROJECT_STANDARD.md` as the primary workflow source of truth.
- Keep `AGENTS.md`, `CLAUDE.md`, and `.github/copilot-instructions.md` thin and aligned with the shared docs instead of duplicating them.
- Put reusable task starters in `.github/prompts/` and path-specific guidance in `.github/instructions/`; do not turn those files into competing standards.
- Update references, commands, and file paths when docs change so handoff material stays accurate.
- Keep comments and guidance explicit about scope, non-goals, and verification expectations.
- When adding AI guidance, prefer repo-specific context packaging over generic role-play prompts.
