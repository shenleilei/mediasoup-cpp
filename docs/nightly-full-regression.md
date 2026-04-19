# Nightly Full Regression

This document describes the repo-local nightly full-regression automation flow.

## Entry Points

- Runner: [scripts/nightly_full_regression.py](../scripts/nightly_full_regression.py)
- Cron installer: [scripts/install_nightly_full_regression_cron.sh](../scripts/install_nightly_full_regression_cron.sh)
- Runtime config example: [/.nightly-full-regression.env.example](../.nightly-full-regression.env.example)

## What The Nightly Runner Does

`scripts/nightly_full_regression.py run`:

- delegates actual test execution to `scripts/run_all_tests.sh`
- creates a timestamped run directory under `artifacts/nightly-full-regression/`
- saves the full raw log for that run
- refreshes the latest-log copy, defaulting to `/var/log/run_all_tests.log`
- snapshots selected Markdown reports into the run directory
- builds an email body with:
  - overall status
  - task pass rate
  - failed tasks
  - best-effort failed cases parsed from the raw log
- sends the email through configured SMTP or local `mailx`

## Run Artifacts

Each run writes:

- `artifacts/nightly-full-regression/<timestamp>/run_all_tests.log`
- `artifacts/nightly-full-regression/<timestamp>/email-body.txt`
- `artifacts/nightly-full-regression/<timestamp>/summary.json`
- `artifacts/nightly-full-regression/<timestamp>/attachments/*.md`

The latest run directory is also exposed as:

- `artifacts/nightly-full-regression/latest`

## Default Markdown Attachments

When these files exist after the run, the runner snapshots and attaches them:

- `docs/full-regression-test-results.md`
- `docs/uplink-qos-case-results.md`
- `docs/plain-client-qos-case-results.md`
- `docs/downlink-qos-test-results-summary.md`
- `docs/downlink-qos-case-results.md`

Missing files are reported in the email body and `summary.json`.

## Config

Copy the example file and edit the local runtime values:

```bash
cd /root/mediasoup-cpp
cp .nightly-full-regression.env.example .nightly-full-regression.env
```

Important keys:

- `MAIL_TO`: recipient list for the nightly email
- `MAIL_FROM`: sender address
- `MAIL_TRANSPORT`: `auto`, `smtp`, or `mailx`
- `SMTP_HOST` / `SMTP_PORT` / `SMTP_USERNAME` / `SMTP_PASSWORD`: SMTP settings when using SMTP
- `NIGHTLY_ARTIFACT_ROOT`: where per-run directories are stored
- `NIGHTLY_LATEST_LOG_PATH`: latest-log copy path
- `RUN_ALL_TESTS_ARGS`: optional extra args forwarded to `scripts/run_all_tests.sh`

## Manual Dry Run

Safe local verification without launching a new full regression:

```bash
cd /root/mediasoup-cpp
./scripts/nightly_full_regression.py run \
  --skip-tests \
  --source-log /var/log/run_all_tests.log \
  --source-report docs/full-regression-test-results.md \
  --no-mail \
  --print-summary
```

## Install The 03:00 Cron Job

Install or refresh the managed cron entry:

```bash
cd /root/mediasoup-cpp
./scripts/install_nightly_full_regression_cron.sh
```

Print the cron line without installing it:

```bash
./scripts/install_nightly_full_regression_cron.sh --print-only
```

The installed job defaults to:

- schedule: `0 3 * * *`
- cron launcher log: `/var/log/mediasoup-cpp-nightly-cron.log`
- runner config: `/root/mediasoup-cpp/.nightly-full-regression.env`

## Notes

- The nightly wrapper keeps local artifacts even if tests fail or email delivery fails.
- The email summary prefers `docs/full-regression-test-results.md` for task counts and falls back to raw-log parsing when needed.
- Failed-case extraction is best effort because not every failing task prints individual case names.
