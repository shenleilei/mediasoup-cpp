# Design

## Context

The repository already has:

- `scripts/run_all_tests.sh` as the accepted full-regression entrypoint
- generated Markdown reports such as `docs/full-regression-test-results.md`
- local cron availability on the target machine

What is missing is an operational wrapper that runs the full regression unattended, stores durable per-run artifacts, and sends a concise summary email.

## Goals

- reuse `scripts/run_all_tests.sh` as the single source of truth for actual full-regression execution
- create durable per-run artifact directories with raw logs and report snapshots
- summarize overall status, pass rate, failed tasks, and failed cases in an email-friendly format
- attach selected Markdown reports from the completed run
- provide an idempotent install path for a 03:00 daily cron schedule

## Proposed Approach

### 1. Add a Python nightly wrapper for execution, parsing, and email assembly

Create `scripts/nightly_full_regression.py` with a `run` subcommand.

Responsibilities:

- load runtime configuration from an untracked env file plus environment variables
- create a timestamped run directory under `artifacts/nightly-full-regression/<timestamp>/`
- run `scripts/run_all_tests.sh` with combined stdout/stderr capture
- write the full output to:
  - the run-specific log file
  - an optional "latest" log path such as `/var/log/run_all_tests.log`
- snapshot selected Markdown report files into the run directory
- parse the structured report and raw log to produce an email summary
- send the email through:
  - configured SMTP first
  - `mailx` fallback if SMTP is not configured
- persist local metadata such as `summary.json` and `email-body.txt` even if mail delivery fails

Python is the best fit here because MIME attachments, report parsing, and fallback email transports are cumbersome and less portable in shell alone.

### 2. Treat `docs/full-regression-test-results.md` as the primary summary source

The wrapper should not re-derive task pass/fail counts from raw logs when the generated Markdown report is available.

Primary parsed fields:

- generated time
- overall status
- attempted tasks
- passed tasks
- failed tasks
- failed groups
- failed task rows from the failed-task summary table

Pass rate is computed as:

- `passed_tasks / attempted_tasks * 100`

If the Markdown report is absent, the wrapper falls back to wrapper-level metadata and best-effort task extraction from the raw log.

### 3. Parse failed cases from the raw log with best-effort patterns

Raw log parsing should extract distinct failed-case style entries in order, including:

- `case <id> failed: <reason>`
- `[<id>] FAIL ...`
- gtest-style `[  FAILED  ] Suite.Test`

The parser intentionally reports these as "failed cases (best effort)" because not every failing task prints individual case names.

### 4. Snapshot Markdown attachments into the run directory

Attach snapshot copies rather than the live docs files.

Default attachment list:

- `docs/full-regression-test-results.md`
- `docs/uplink-qos-case-results.md`
- `docs/plain-client-qos-case-results.md`
- `docs/downlink-qos-test-results-summary.md`
- `docs/downlink-qos-case-results.md`

At run end, existing files are copied into:

- `artifacts/nightly-full-regression/<timestamp>/attachments/`

This ensures the emailed artifacts and local run directory remain consistent even after later runs overwrite the docs files.

### 5. Provide an idempotent cron installer

Create `scripts/install_nightly_full_regression_cron.sh`.

Responsibilities:

- resolve the repo root
- choose the default config file path under the repo root
- remove any old cron line managed by this installer
- install a single `0 3 * * *` entry that calls the nightly wrapper
- keep a small cron launcher log separate from the per-run full log

The actual test output still lives in the run-specific log directories plus the configured latest log copy.

### 6. Keep secrets out of git

Commit:

- the wrapper script
- the cron install script
- a documented example env file

Do not commit:

- the live runtime env file
- SMTP credentials
- per-run artifacts

Use an untracked runtime env file such as:

- `.nightly-full-regression.env`

## Data And Artifacts

Per-run directory:

- `run_all_tests.log`
- `summary.json`
- `email-body.txt`
- `attachments/<copied markdown files>`

Optional latest-path copy:

- `/var/log/run_all_tests.log`

## Failure Handling

- If `scripts/run_all_tests.sh` exits non-zero, the wrapper still snapshots reports, builds the email body, and attempts mail delivery
- If attachment files are missing, the wrapper records them in the email body and local summary instead of aborting
- If SMTP delivery fails and `mailx` fallback is unavailable or also fails, the wrapper exits non-zero after saving local artifacts
- If the latest log path is unwritable, the wrapper records that condition in local metadata but does not discard the run-specific log

## Alternatives Considered

### Alternative: put `./scripts/run_all_tests.sh > /var/log/run_all_tests.log 2>&1` directly into cron

Rejected because it would:

- overwrite the only durable log
- lose per-run report snapshots
- require ad hoc parsing directly from the latest log only
- provide no structured email assembly or local metadata on mail failure

### Alternative: implement the wrapper entirely in shell

Rejected because MIME attachment handling, robust log parsing, and transport fallback are easier to keep correct and maintainable in Python.

### Alternative: use systemd timer units instead of cron

Rejected for this change because cron is already installed and the smallest repo-local installation flow is a managed crontab entry. A future change can add systemd units if the deployment standard moves there.

## Testing Strategy

- `python3 -m py_compile scripts/nightly_full_regression.py`
- `bash -n scripts/install_nightly_full_regression_cron.sh`
- run the nightly wrapper in a dry-run or no-mail mode against fixture-like inputs or a targeted invocation
- verify the cron installer is idempotent by running it twice and checking the managed entry count
- verify docs and accepted specs reflect the new nightly workflow and artifact paths
