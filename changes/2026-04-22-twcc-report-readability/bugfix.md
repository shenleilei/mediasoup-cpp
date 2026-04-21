# Bugfix Analysis

## Summary

The current generated TWCC A/B reports are technically complete but not acceptable as reader-facing reports.

The main defects are:

- the pairwise reports read like raw scoreboards instead of explaining what is being compared
- `PASS` / `FAIL` is easy to misread as “feature works / feature is broken” rather than “all configured gates passed / at least one gate did not pass”
- the reports do not clearly explain that `G0`, `G1`, and `G2` are runtime-mode comparisons, often on the same commit
- the stable summary page is too thin, so readers have to jump directly into dense pairwise tables without context
- there is no standalone summary re-render path, so improving the summary output currently implies rerunning the full experiment

## Reproduction

1. Open:
   - `docs/generated/twcc-ab-report.md`
   - `docs/generated/twcc-ab-g0-vs-g2.md`
   - `docs/generated/twcc-ab-g1-vs-g2.md`
2. Try to answer:
   - what is being compared
   - whether `FAIL` means a runtime breakage or only a gate miss
   - which sections should be read first
3. Compare that reading experience with `docs/twcc-ab-test.md`.

## Observed Behavior

- Pairwise reports start immediately with metric tables and do not orient the reader first.
- A report can show unchanged goodput/loss and improved recovery, yet still show overall `FAIL` with little human explanation.
- The reports show the same commit for baseline and candidate without explaining why that is valid.
- The stable summary page lists pairwise outputs but does not summarize the practical meaning of the current result set.

## Expected Behavior

- Generated TWCC reports SHALL explain the comparison intent before presenting dense metrics.
- Generated TWCC reports SHALL explain what `G0`, `G1`, and `G2` mean in runtime terms.
- Generated TWCC reports SHALL explain that overall `FAIL` means “did not satisfy every configured gate”, not automatically “runtime path is broken”.
- The stable summary page SHALL provide a short reader-facing interpretation before linking to pairwise detail.
- TWCC summary rendering SHALL be re-runnable from existing JSON artifacts without rerunning the full A/B experiment.

## Known Scope

- `tests/qos_harness/render_twcc_ab_report.mjs`
- `tests/qos_harness/run_twcc_ab_eval.mjs`
- new summary renderer under `tests/qos_harness/`
- `docs/twcc-ab-test.md`
- current stable generated TWCC reports under `docs/generated/`
- latest timestamped TWCC artifact docs under
  `changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T20-01-09.632Z/`

## Must Not Regress

- current TWCC metric calculations and gate semantics
- existing artifact paths and stable report entrypoints
- accepted scope documented in `docs/plain-client-qos-status.md` and `specs/current/plain-client-send-side-bwe.md`
- ability for full QoS regression to refresh stable TWCC docs

## Suspected Root Cause

High confidence: the original report generator optimized for data completeness and machine-oriented review, but report readability was never treated as an explicit delivery requirement.

## Acceptance Criteria

### Requirement 1

Pairwise TWCC reports SHALL become reader-facing documents.

#### Scenario: reader opens `g0-vs-g2` or `g1-vs-g2`

- WHEN a reader opens a pairwise TWCC report
- THEN the report first explains what the baseline and candidate runtime modes are
- AND the report explains what overall `PASS` / `FAIL` means
- AND the report provides a short plain-language interpretation of the current result

### Requirement 2

The stable TWCC summary SHALL become a usable entrypoint.

#### Scenario: reader opens the latest stable summary

- WHEN a reader opens `docs/generated/twcc-ab-report.md`
- THEN the page explains how to read the result set
- AND it summarizes the practical meaning of each pairwise comparison before linking to detail

### Requirement 3

TWCC summary rendering SHALL be decoupled from full experiment execution.

#### Scenario: report wording refresh

- WHEN a developer needs to improve the stable TWCC summary wording
- THEN the summary can be re-rendered from existing JSON artifacts
- WITHOUT rerunning the full TWCC A/B experiment

## Regression Expectations

- Required automated regression coverage:
  - re-render current TWCC pairwise and summary docs from existing JSON inputs
  - `git diff --check`
- Required manual smoke checks:
  - spot-read `docs/generated/twcc-ab-report.md`
  - spot-read `docs/generated/twcc-ab-g0-vs-g2.md`
  - spot-read `docs/generated/twcc-ab-g1-vs-g2.md`
  - spot-read the latest timestamped `twcc-ab-eval.md`
