# Bugfix Design

## Context

This is a documentation/tooling bugfix, not a transport-control behavior change.

The metrics and gate logic are already present. The problem is that the generated output is hard to read unless the reader already knows:

- what `G0`, `G1`, `G2` mean
- why baseline and candidate may share the same commit
- why overall `FAIL` can coexist with non-regressing top-line metrics
- which sections of the report matter first

## Root Cause

The current generators separate responsibilities unevenly:

- `render_twcc_ab_report.mjs` renders dense pairwise markdown from one JSON metrics file
- `run_twcc_ab_eval.mjs` embeds summary-page rendering inline

That led to two issues:

1. readability logic was never centralized as an explicit requirement
2. the summary page cannot be re-rendered independently from full experiment execution

## Fix Strategy

### 1. Make Pairwise Reports Reader-Facing

Extend `render_twcc_ab_report.mjs` so each pairwise report includes:

- document nature / what this report is for
- comparison context for the baseline and candidate labels
- explicit explanation of `PASS` / `FAIL`
- a short “how to read this report” section
- a plain-language summary of the current result set before the dense tables
- a short scene/AB-case guide so the reader knows what each case is testing

The metrics, gate tables, and detailed per-scenario sections remain, but they move behind clearer framing.

### 2. Extract Summary Rendering

Create a standalone summary renderer under `tests/qos_harness/` that can produce:

- the stable summary at `docs/generated/twcc-ab-report.md`
- the run-local summary at `<timestamp>/twcc-ab-eval.md`

Inputs should come from already generated pairwise JSON reports and their artifact metadata, so the summary can be refreshed without rerunning the experiment itself.

`run_twcc_ab_eval.mjs` will call this renderer after producing pairwise metrics.

### 3. Refresh Usage Guidance

Update `docs/twcc-ab-test.md` so it explains the new report structure and the recommended reading order:

1. stable summary
2. pairwise report
3. raw JSON / trace only when deeper debugging is needed

## Scope Control

This change does NOT:

- alter metric derivation
- alter gate thresholds
- change scenario selection
- rerun the transport experiment for new measurements

The only generated-content update is re-rendering from the current JSON artifacts.

## Verification Strategy

### Renderer Verification

- re-render the current pairwise reports from:
  - `docs/generated/twcc-ab-g0-vs-g2.json`
  - `docs/generated/twcc-ab-g1-vs-g2.json`
- re-render the current summary reports from the same latest artifact set

### Readability Verification

- confirm a new reader can answer from each report:
  - what is being compared
  - what `FAIL` means
  - what the main practical conclusion is

### Hygiene Verification

- `git diff --check`

## Risks

- over-explaining could bury the actual metrics under too much prose
- summary rendering extraction could accidentally diverge from the artifact paths used by the full experiment script

## Mitigation

- keep new prose front-loaded and concise
- preserve the existing artifact naming and linking scheme
- regenerate the current stable and latest timestamped docs from the real artifact paths after the code change
