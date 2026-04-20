# Design

## Context

The repository already has two kinds of date-based backups:

- nightly run directories under `artifacts/nightly-full-regression/<timestamp>/`
- report archives under `docs/archive/.../<timestamp>/`

It also already refreshes live report docs during full-regression runs.

What is missing is:

- bounded retention for those timestamped directories
- git-visible recording of the refreshed docs

## Proposed Approach

### 1. Add bounded retention helpers for timestamped backup directories

Add a small helper that:

- lists direct child directories under a backup root
- ignores non-directories and symlinks such as `latest`
- sorts timestamped directory names lexicographically
- removes the oldest entries beyond a fixed max count of 100

Apply it to:

- `artifacts/nightly-full-regression/` in the Python nightly wrapper
- shared QoS report archive helpers in `tests/qos_harness/report_artifacts.mjs`
- the downlink archive path in `tests/qos_harness/run_downlink_matrix.mjs`

The existing date-like directory names already sort chronologically, so lexicographic order is sufficient.

### 2. Add nightly git recording for refreshed docs

Inside `scripts/nightly_full_regression.py`:

- capture pre-run git status for doc paths
- after the run, gather post-run git status for `docs/`
- identify doc paths that became dirty during the run and were not already dirty beforehand
- stage only those newly dirty doc paths
- create a git commit with a nightly-specific message

The commit message should encode:

- nightly run id
- pass/fail status

This keeps the history understandable without re-parsing the report.

### 3. Exclude pre-existing dirty docs from auto-commit

To avoid sweeping unrelated local work into a nightly commit:

- any `docs/` path already dirty before the run is excluded from auto-stage
- skipped paths are written into `summary.json`
- the email/run summary can mention whether the nightly doc commit was created, skipped, or failed

This is a narrow safeguard that still lets nightly-generated docs be recorded.

### 4. Limit commit scope to docs

The wrapper should only auto-record doc/report outputs, not arbitrary repo changes.

This change intentionally does not auto-stage:

- source files
- test binaries
- local config files
- nightly artifact directories

The purpose is to record report surfaces, not to turn nightly automation into a general repo sync bot.

## Verification Strategy

- `python3 -m py_compile scripts/nightly_full_regression.py`
- `node tests/qos_harness/test.report_artifacts.mjs`
- targeted dry-run style invocation of the nightly wrapper with `--skip-tests` and existing log/report inputs
- verify updated docs/specs describe git recording and 100-entry retention

## Risks

- auto-commit behavior can be surprising if a nightly machine is not kept clean; the pre-existing-dirty-path exclusion mitigates this but does not remove all workflow assumptions
- retention pruning removes older local backup directories; this is intentional, but the rule must be documented clearly
- downlink archive code currently uses a custom archive implementation, so retention must be wired there separately or it will drift
