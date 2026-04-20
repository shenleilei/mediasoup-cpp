# Design

## Context

The request is a whole-repository review rather than a behavior change. The work is non-trivial and should be tracked as a Structured Change per `PROJECT_STANDARD.md`.

## Proposed Approach

1. Establish review inputs from workflow standards and accepted specs.
2. Perform whole-system code inspection with focus on correctness, lifecycle safety, contracts, failure handling, and test coverage.
3. Run key verification commands (build/test entrypoints feasible in this run) and capture outcomes.
4. Produce a repository review report with findings-first structure per `REVIEW.md`.

## Alternatives Considered

- Limit review to currently modified files only.
  - Rejected because the user asked for whole-repository review and standards require system-level perspective.

- Produce chat-only findings without writing documentation.
  - Rejected because the user requested a recorded report artifact.

## Modules And Responsibilities

- `src/`: primary runtime/backend risk surface (highest priority).
- `tests/`: validation quality and regression coverage.
- `scripts/` and build files: verification and delivery reliability.
- `docs/`: destination for final review report.

## Data And State

- No runtime data or schema changes.
- Artifacts created are documentation-only (`changes/` docs and final review report).

## Interfaces

- No API contract changes.
- Output interface is a markdown report for maintainers.

## Failure Handling

- If a command cannot be run, record the exact gap and reason in the report.
- Candidate issues without enough proof must be marked as open questions instead of confirmed defects.

## Security Considerations

- Review includes explicit check for sensitive-data handling and unsafe runtime paths where visible.

## Testing Strategy

- No code behavior changes expected.
- Execute available build/test commands as review evidence and gap detection.

## Observability

- Record executed commands and outcomes in the final report.

## Rollout Notes

- Documentation-only deliverable; no rollout or migration required.
