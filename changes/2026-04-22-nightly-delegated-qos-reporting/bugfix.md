# Bugfix Analysis

## Summary

The nightly full-regression wrapper currently produces a report/email summary that is too coarse for the delegated QoS surface.

Specifically:

- `scripts/run_all_tests.sh` records only the top-level delegated task `qos:qos-regression`
- the nightly wrapper email and summary therefore show only a few coarse items
- readers cannot see the delegated QoS task set such as:
  - `cpp-client-matrix`
  - `cpp-client-ab`
  - `cpp-threaded:gtest`
  - `browser-harness:*`
  - `node-harness:*`
  - `matrix`
  - `downlink-matrix`

## Reproduction

1. Inspect an existing nightly run summary/email body generated from:
   - `artifacts/nightly-full-regression/<timestamp>/run_all_tests.log`
   - `docs/full-regression-test-results.md`
2. Observe that the summary shows the top-level `qos` task, but not the delegated QoS task set underneath it.

## Observed Behavior

- The nightly wrapper parses the top-level markdown report correctly.
- It also parses case counts from the raw log.
- But it does not surface delegated QoS task execution rows in the rendered summary/email.

## Expected Behavior

- The nightly wrapper SHALL surface delegated QoS task rows parsed from the raw log.
- The reader SHALL be able to tell whether `cpp-client-matrix`, `cpp-client-ab`, `cpp-threaded`, and other delegated QoS tasks ran and whether they passed or failed.

## Known Scope

- `scripts/nightly_full_regression.py`
- current nightly artifact outputs under `artifacts/nightly-full-regression/`

## Must Not Regress

- current parsing of the top-level full-regression markdown report
- current failed-case extraction
- current email delivery and artifact snapshot flow

## Acceptance Criteria

### Requirement 1

Delegated QoS task rows SHALL be extracted from the raw log.

#### Scenario: nightly run includes delegated QoS execution

- WHEN the raw log contains nested `==> [label]` / `<== [label] ...` blocks under `qos:qos-regression`
- THEN the nightly wrapper extracts those delegated task rows with status and duration

### Requirement 2

Nightly summaries SHALL render delegated QoS tasks.

#### Scenario: reader inspects nightly summary

- WHEN a nightly summary is rendered
- THEN it contains a section that lists delegated QoS task rows
- AND that section includes entries such as `cpp-client-matrix` and `cpp-client-ab` when present in the log

## Regression Expectations

- Required validation:
  - run the nightly wrapper in `--skip-tests` mode against an existing run log/report
  - confirm delegated QoS task rows appear in the rendered summary
