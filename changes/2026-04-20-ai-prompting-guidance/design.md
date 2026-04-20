# Design

## Context

The repository already has a strong AI workflow structure:

- `docs/aicoding/PROJECT_STANDARD.md` defines the main workflow and delivery gates
- `docs/aicoding/PLANS.md` defines execution planning
- `docs/aicoding/REVIEW.md` defines review and self-review expectations
- `AGENTS.md` and `.github/copilot-instructions.md` act as thin entrypoints

What is missing is a repo-local operating layer for AI-assisted work inside IDEs:

- how to package context for common tasks
- how to use repository-wide versus path-specific instructions
- how to reuse prompt templates without turning them into competing standards

## Proposed Approach

### 1. Add a supplemental prompting guide

Create `docs/aicoding/PROMPTING.md` as an additive document that explains:

- the priority order between standards, change docs, instructions, and prompt files
- how to write good repo-specific prompts for planning, review, tests, and explanations
- how to iterate on prompts using failure cases and verification rather than prompt folklore
- what not to put into prompt assets

This keeps prompting advice discoverable without diluting the hard workflow standards.

### 2. Add reusable prompt files for common repo workflows

Create `.github/prompts/*.prompt.md` files for the most common tasks:

- planning a structured change
- whole-system review
- writing regression tests
- explaining runtime flow

Each prompt should:

- reference the controlling repo docs
- ask for explicit output structure where useful
- keep scope narrow and aligned with the project's review and verification expectations

### 3. Add path-specific instruction files

Create `.github/instructions/*.instructions.md` for:

- source and build files
- tests
- docs and AI instruction files

Each file uses `applyTo` frontmatter and gives narrow, area-specific guidance. This avoids bloating `.github/copilot-instructions.md` while keeping repo-wide and path-specific guidance consistent.

### 4. Keep the repo-wide instruction file thin

Update `.github/copilot-instructions.md` and `docs/aicoding/README.md` so developers can discover the new prompting layer, but do not move workflow requirements out of the current standards.

## Alternatives Considered

- Put all prompting advice into `.github/copilot-instructions.md`
  - Rejected because it would turn the repo-wide instruction file into a second standard and increase drift risk

- Import third-party prompt libraries directly into the repository
  - Rejected because most community prompt sets are too broad, role-heavy, or weakly coupled to this repository's workflow and verification model

- Do nothing beyond the existing workflow docs
  - Rejected because the repo currently lacks reusable IDE-facing prompt assets and path-specific instruction surfaces

## Modules And Responsibilities

- `docs/aicoding/PROMPTING.md`: supplemental prompting guidance and context-packaging rules
- `docs/aicoding/README.md`: discoverability for the prompting guide
- `.github/copilot-instructions.md`: thin repo-wide entrypoint that points to the right sources
- `.github/prompts/*.prompt.md`: reusable workflow-specific prompt templates
- `.github/instructions/*.instructions.md`: path-specific instructions for supported tools

## Data And State

- No runtime data model changes
- No schema changes
- No state-machine changes

## Interfaces

- Developer-facing repository documentation changes only
- GitHub Copilot-compatible repository customization files under `.github/`

## Failure Handling

- If a tool ignores prompt files or path-specific instructions, the core workflow docs still remain sufficient
- Prompt files will explicitly direct the tool to consult controlling repo docs when context is missing
- The repo-wide instruction file will continue to point back to the core standards to avoid ambiguity

## Security Considerations

- No secrets, credentials, or runtime auth flows are introduced
- Prompting guidance should discourage hiding repo-critical requirements in chat-only prompts

## Testing Strategy

- Verify that every new instruction file has valid `applyTo` frontmatter
- Verify that prompt files and docs reference the correct repo paths
- Run `git diff --check`
- Perform whole-change self-review against `REVIEW.md` to ensure no conflicting instruction layer was introduced

## Observability

- Not applicable for runtime behavior

## Rollout Notes

- Additive documentation change only
- Existing tools that only read `AGENTS.md` or `.github/copilot-instructions.md` continue to work
- Developers can adopt prompt files and path-specific instructions incrementally
