# Tasks

## 1. Baseline

- [x] 1.1 Define the live-doc scope for this refresh
  Verification: `requirements.md` and `design.md` exist and limit the sweep to repo-maintained live Markdown

## 2. Primary Docs

- [x] 2.1 Refresh top-level and development docs
  Files: `README.md`, `docs/README.md`, `docs/DEVELOPMENT.md`, `specs/current/test-entrypoints.md`
  Verification: primary navigation and test workflow match current code and scripts

- [x] 2.2 Refresh AI-coding workflow docs where current behavior changed
  Files: `AGENTS.md`, `docs/aicoding/*`
  Verification: agent instructions and review/plan expectations remain internally consistent

## 3. Active Change Docs

- [x] 3.1 Refresh affected active change folders to reflect implemented state
  Files: current `changes/2026-04-17-*` and `changes/2026-04-18-*` docs as needed
  Verification: task status and implementation notes match the codebase

## 4. Consistency Sweep

- [x] 4.1 Remove misleading live-doc references to shared Redis-backed tests or outdated monolithic runtime files
  Verification: targeted repo search shows the obsolete guidance is gone from live docs

## 5. Delivery

- [x] 5.1 Run documentation sanity checks and self-review
  Verification: `bash -n` for referenced scripts if touched, plus final whole-system doc review against current code
