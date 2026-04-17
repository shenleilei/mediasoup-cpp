# Research Notes

This template is a synthesis of multiple public approaches to AI-assisted development.

## Patterns Borrowed

### Kiro

Borrowed:

- Structured specs with three core artifacts: requirements, design, and tasks
- Persistent project memory through steering files
- Stronger correctness practices for rules-heavy work

Applied here as:

- `changes/<date>-<name>/requirements.md`
- `changes/<date>-<name>/design.md`
- `changes/<date>-<name>/tasks.md`
- `.kiro/steering/`

### OpenSpec

Borrowed:

- Separate accepted truth from proposed change work
- Keep changes auditable in dedicated folders
- Archive changes after accepted behavior is merged

Applied here as:

- `specs/current/` for accepted behavior
- `changes/` for active work

### Claude Code

Borrowed:

- Project memory belongs in repo files, not only in chat
- Memory should have a hierarchy and remain small and specific

Applied here as:

- `CLAUDE.md` as a thin project entrypoint
- `PROJECT_STANDARD.md` as the main shared memory
- `PLANS.md` as the execution protocol

### Cursor And GitHub Copilot

Borrowed:

- Repository-scoped instructions are useful
- Instruction scope should be explicit
- Conflicting instruction files create unpredictable outcomes

Applied here as:

- `AGENTS.md` for cross-agent instructions
- `.github/copilot-instructions.md` as a thin wrapper
- Minimal duplication across tool-specific files

### AGENTS.md

Borrowed:

- A simple, open, predictable location for agent instructions

Applied here as:

- `AGENTS.md` as the operational entrypoint for coding agents

## Decisions We Made

We did not copy any single tool workflow wholesale.

Instead, we defined a house style:

- One primary standard file
- One execution planning protocol
- One review protocol
- Thin tool adapters
- Specs for non-trivial work
- Accepted specs separate from active changes
- Tests and delivery gates as completion criteria
- Small, reviewable tasks

## Why This Is Suitable For Small Teams

- Low process overhead for tiny edits
- Enough structure for risky changes
- Compatible with Kiro, Claude Code, Copilot, and generic coding agents
- Works for greenfield and brownfield projects

## Source Links

- Kiro Specs: https://kiro.dev/docs/specs/
- Kiro Steering: https://kiro.dev/docs/steering/
- Kiro Correctness: https://kiro.dev/docs/specs/correctness/
- OpenSpec: https://openspec.dev/
- Claude Code Memory: https://docs.anthropic.com/en/docs/claude-code/memory
- GitHub Copilot Repository Instructions: https://docs.github.com/en/copilot/how-tos/configure-custom-instructions/add-repository-instructions
- AGENTS.md: https://agents.md/
