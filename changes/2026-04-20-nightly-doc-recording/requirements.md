# Requirements

## Summary

Extend the repo-local nightly full-regression workflow so it records refreshed documentation in git and keeps date-based backup directories bounded.

## Business Goal

Developers want nightly full-regression results to leave a durable git-visible record of the refreshed report docs, while preventing date-based backup directories from growing without bound.

## In Scope

- committing refreshed nightly report docs after a nightly run
- preserving failure runs in that git record instead of only recording passing runs
- skipping pre-existing dirty doc paths so nightly automation does not absorb unrelated local edits
- retaining only the most recent 100 timestamped backup directories for nightly artifacts and date-based report archives
- updating specs and runbook docs for the new nightly behavior

## Out Of Scope

- changing which tests `scripts/run_all_tests.sh` or `scripts/run_qos_tests.sh` execute
- automatically pushing commits to a remote
- rewriting historical archive layout beyond bounded retention
- committing non-doc files produced outside the report/summary surface

## User Stories

### Story 1

As a developer
I want the nightly full-regression wrapper to commit refreshed report docs
So that each run leaves a git-visible record even when the run fails.

### Story 2

As a developer
I want date-based backup directories to stay bounded
So that long-running nightly automation does not accumulate unlimited local history.

### Story 3

As a developer
I want nightly git recording to avoid unrelated dirty docs that existed before the run
So that automation does not accidentally commit in-progress local work.

## Acceptance Criteria

### Requirement 1

The nightly wrapper SHALL record refreshed docs in git after a run.

#### Scenario: report docs changed during the run

- WHEN `scripts/nightly_full_regression.py run` completes
- THEN it SHALL detect newly changed doc paths produced by that run
- AND it SHALL create a git commit for those doc changes
- AND it SHALL do so even when the test run exits non-zero

#### Scenario: pre-existing dirty docs

- WHEN matching doc paths were already dirty before the nightly run started
- THEN the nightly wrapper SHALL NOT auto-stage those pre-existing dirty paths
- AND it SHALL record that skip decision in local run metadata

### Requirement 2

Date-based backup directories SHALL be retained by recency.

#### Scenario: nightly artifact retention

- WHEN a nightly run creates a new timestamped directory under `artifacts/nightly-full-regression/`
- THEN the workflow SHALL keep only the most recent 100 timestamped run directories
- AND it SHALL preserve the `latest` symlink behavior

#### Scenario: report archive retention

- WHEN report archive code writes a new timestamped directory under a date-based archive root
- THEN that archive root SHALL keep only the most recent 100 timestamped report directories

### Requirement 3

The new recording behavior SHALL be documented as part of the accepted nightly workflow.

#### Scenario: runbook and spec update

- WHEN the change is complete
- THEN `specs/current/test-entrypoints.md` and `docs/nightly-full-regression.md` SHALL describe
  - nightly git recording of refreshed docs
  - the bounded 100-run backup retention rule

## Non-Functional Requirements

- Safety: nightly git recording must not silently stage pre-existing dirty doc paths
- Durability: failed nightly runs must still preserve logs, backup snapshots, and git-visible doc changes when available
- Maintainability: retention logic should live close to the archive creation paths instead of as an external cleanup job

## Edge Cases

- A nightly run may produce no new doc changes; the wrapper should report “no commit needed” instead of failing
- Git may be unavailable or the commit may fail; the wrapper must still persist local artifacts and summary metadata
- Backup roots may contain symlinks such as `latest`; retention must prune only timestamped run directories
