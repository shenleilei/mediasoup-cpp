# Tasks

## 1. Reproduce

- [ ] 1.1 Capture the current readability defects in the stable and pairwise TWCC reports
  Files: `docs/generated/twcc-ab-report.md`, `docs/generated/twcc-ab-g0-vs-g2.md`, `docs/generated/twcc-ab-g1-vs-g2.md`
  Verification: note the missing comparison context, ambiguous `FAIL`, and weak reading guidance

## 2. Fix Pairwise Rendering

- [ ] 2.1 Make pairwise TWCC reports reader-facing without changing the underlying metrics
  Files: `tests/qos_harness/render_twcc_ab_report.mjs`
  Verification: re-render both stable pairwise reports from current JSON inputs

## 3. Fix Summary Rendering

- [ ] 3.1 Extract a standalone TWCC summary renderer
  Files: new summary renderer under `tests/qos_harness/`, `tests/qos_harness/run_twcc_ab_eval.mjs`
  Verification: re-render the stable summary and latest timestamped summary from existing JSON artifacts

## 4. Update Reader Guidance

- [ ] 4.1 Refresh TWCC report usage guidance
  Files: `docs/twcc-ab-test.md`
  Verification: manual read-through against the regenerated reports

## 5. Regenerate Reports

- [ ] 5.1 Re-render current stable TWCC reports from existing artifacts
  Files: `docs/generated/twcc-ab-report.md`, `docs/generated/twcc-ab-g0-vs-g2.md`, `docs/generated/twcc-ab-g1-vs-g2.md`
  Verification: regenerated docs reflect the new structure without changing measured values

- [ ] 5.2 Re-render the latest timestamped TWCC report docs from existing artifacts
  Files: `changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T20-01-09.632Z/*`
  Verification: timestamped docs reflect the new structure without rerunning the experiment

## 6. Delivery Gates

- [ ] 6.1 Run `git diff --check`
  Verification: command exits cleanly

- [ ] 6.2 Review the changed TWCC report/docs set from the reader perspective
  Verification: manual smoke review confirms the new wording answers what is being compared, how to read it, and what the current result means
