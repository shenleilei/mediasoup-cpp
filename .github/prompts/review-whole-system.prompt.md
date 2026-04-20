# Review From The Whole-System Perspective

Use this prompt when reviewing a branch, patch, or non-trivial diff in this repository.

Read first:

- [`docs/aicoding/REVIEW.md`](../../docs/aicoding/REVIEW.md)
- [`docs/aicoding/PROJECT_STANDARD.md`](../../docs/aicoding/PROJECT_STANDARD.md)
- [`docs/aicoding/DELIVERY_CHECKLIST.md`](../../docs/aicoding/DELIVERY_CHECKLIST.md)
- the active change docs under `changes/`
- affected files under `specs/current/` and `docs/`
- the full changed file set, not just one visible diff hunk

Task:

1. Review the change from the overall system perspective.
2. Check whether the change is reviewable as submitted or should be split because of mixed scope or excessive breadth.
3. Check correctness, lifecycle, retries, cleanup, contract boundaries, missing tests, and documentation drift.
4. Inspect shared infrastructure or foundational modules when the branch behavior depends on them indirectly.
5. Identify missing design-stage review for significant architecture, protocol, API, or workflow changes.
6. Identify missing qualified reviewer coverage for concurrency, protocol, security, build, crash-recovery, or performance-critical areas.
7. Treat missing verification and partial cross-boundary updates as findings.
8. For higher-risk C++ backend changes, treat missing sanitizer-like, crash-recovery, stress, or benchmark evidence as a verification gap when applicable.

Output format:

1. Findings, ordered by severity, each with:
   - severity
   - impact
   - file reference
   - why it is a problem
2. Open questions or assumptions
3. Verification gaps or residual risks
4. Brief change summary

Constraints:

- Findings come before summary.
- If there are no findings, state that explicitly and still call out residual risks or test gaps.
- Do not anchor only on the files already mentioned in chat.
