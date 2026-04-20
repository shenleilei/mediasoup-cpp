# Tasks

## 1. Define The Change

- [x] 1.1 Document prompting guidance requirements and design
  Files: `changes/2026-04-20-ai-prompting-guidance/requirements.md`, `changes/2026-04-20-ai-prompting-guidance/design.md`, `changes/2026-04-20-ai-prompting-guidance/tasks.md`
  Verification: change docs describe scope, non-goals, and verification

## 2. Add Supplemental Prompting Guidance

- [x] 2.1 Add a prompting guide under `docs/aicoding/`
  Files: `docs/aicoding/PROMPTING.md`, `docs/aicoding/README.md`
  Verification: prompting guide states priority order and additive role clearly

## 3. Add Reusable Prompt Assets

- [x] 3.1 Add repo-specific prompt files for planning, review, regression tests, and runtime explanation
  Files: `.github/prompts/*.prompt.md`
  Verification: prompt files reference the controlling repo docs and requested output shape

## 4. Add Path-Specific Instructions

- [x] 4.1 Add path-specific instruction files for source/build, tests, and docs
  Files: `.github/instructions/*.instructions.md`
  Verification: each file has `applyTo` frontmatter and narrow, area-specific guidance

- [x] 4.2 Keep the repo-wide entrypoints thin and discoverable
  Files: `.github/copilot-instructions.md`, `docs/aicoding/README.md`
  Verification: entrypoint docs mention the new prompting surfaces without duplicating core workflow rules

## 5. Verify

- [x] 5.1 Run documentation-focused verification
  Verification:
  - `git diff --check`
  - `find .github/prompts -maxdepth 1 -name '*.prompt.md' | sort`
  - `find .github/instructions -maxdepth 1 -name '*.instructions.md' | sort`
  - `rg -n '^applyTo:' .github/instructions`

- [x] 5.2 Run self-review using `docs/aicoding/REVIEW.md`
  Verification: whole-change review confirms no instruction drift or missing discoverability updates
