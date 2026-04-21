# Bugfix Analysis

## Summary

The repository root `README.md` contains a reader-facing “Current checked status” block, but that block is currently static.

As a result:

- daily/nightly regression refreshes can update the generated QoS artifacts
- yet the root README still shows stale browser and plain-client matrix status lines
- and nightly auto-doc commits do not record root `README.md` changes even if the file is updated

## Reproduction

1. Open `README.md` and note the matrix status lines under “Current checked status”.
2. Compare them with:
   - `docs/generated/uplink-qos-matrix-report.json`
   - `docs/generated/uplink-qos-cpp-client-matrix-report.json`
3. Observe that the root README can lag behind the generated artifacts.

## Observed Behavior

- Root README status lines are manually maintained.
- `scripts/run_all_tests.sh` rewrites `docs/full-regression-test-results.md`, but not the root README status block.
- `scripts/nightly_full_regression.py` only auto-records newly changed `docs/` paths, so a refreshed root `README.md` would not be committed by nightly.

## Expected Behavior

- After a QoS-inclusive full regression run, the root `README.md` SHALL refresh its browser and plain-client matrix status lines from the latest generated matrix JSON artifacts.
- Nightly auto-doc recording SHALL include a newly changed root `README.md` together with `docs/` changes.

## Known Scope

- `README.md`
- `scripts/run_all_tests.sh`
- `scripts/nightly_full_regression.py`
- `specs/current/test-entrypoints.md`

## Must Not Regress

- existing generated report output paths
- nightly artifact capture and email flow
- `docs/full-regression-test-results.md` generation

## Acceptance Criteria

### Requirement 1

The root README status block SHALL be refreshed automatically.

#### Scenario: full regression run updates matrix JSONs

- WHEN `scripts/run_all_tests.sh` runs a QoS-inclusive regression
- THEN `README.md` updates the browser and plain-client matrix status lines from the latest generated JSON artifacts
- AND the date label comes from the corresponding `generatedAt` field

### Requirement 2

Nightly git recording SHALL include the root README.

#### Scenario: nightly run updates README

- WHEN the nightly wrapper updates `README.md`
- THEN the file is included in the nightly auto-recorded git change set

## Regression Expectations

- Required automated checks:
  - `bash -n scripts/run_all_tests.sh`
  - `python3 -m py_compile scripts/nightly_full_regression.py`
  - manual refresh of the README status block from current JSON artifacts
