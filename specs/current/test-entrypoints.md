# Test Entrypoints

## `scripts/run_all_tests.sh`

- `scripts/run_all_tests.sh` is the full repository regression entrypoint.
- It SHALL continue running remaining selected groups after one selected group fails.
- It SHALL exit non-zero after the run if any selected group failed.
- It SHALL cover every C++ gtest target defined in `CMakeLists.txt`.
- It SHALL also cover the full QoS regression surface defined by `scripts/run_qos_tests.sh`.
- After an actual test execution, it SHALL rewrite `docs/full-regression-test-results.md`.
- When its selected groups delegate to `scripts/run_qos_tests.sh`, the QoS-specific reports owned by that script SHALL be refreshed according to that script's contract.
- The generated Markdown report SHALL include:
  - generation time
  - selected groups for that run
  - overall pass/fail status
  - failed-group summary
  - per-task status and elapsed time for each attempted task
- `--help` and `--list` SHALL exit without rewriting `docs/full-regression-test-results.md`.

## `scripts/run_qos_tests.sh`

- `scripts/run_qos_tests.sh` is the QoS regression entrypoint.
- It SHALL continue running remaining selected groups after one selected task or group fails.
- Its C++ unit and C++ integration groups SHALL execute the dedicated QoS binaries without per-case whitelist filters.
- It SHALL record failed tasks in `tests/qos_harness/artifacts/last-failures.txt`.
- After an actual execution, it SHALL rewrite `docs/downlink-qos-test-results-summary.md`.
- When matrix-style groups run, it SHALL regenerate the corresponding QoS Markdown reports documented in `README.md`.

## `scripts/nightly_full_regression.py`

- `scripts/nightly_full_regression.py run` is the repo-local nightly full-regression wrapper.
- It SHALL delegate actual test execution to `scripts/run_all_tests.sh`.
- It SHALL create a timestamped run directory under `artifacts/nightly-full-regression/`.
- It SHALL retain only the most recent 100 timestamped nightly run directories under that artifact root.
- It SHALL preserve the full raw log for each run.
- It SHALL refresh the configured latest-log copy path after each run.
- It SHALL snapshot the configured Markdown report attachments into the run directory.
- It SHALL auto-record newly changed `docs/` paths from the nightly run in git.
- It SHALL exclude `docs/` paths that were already dirty before the run started.
- It SHALL render an email body that includes:
  - overall status
  - task pass rate when `docs/full-regression-test-results.md` is available
  - failed tasks
  - best-effort failed cases parsed from the raw log
- It SHALL preserve local artifacts even when tests fail or email delivery fails.

## Date-Based Report Archives

- Date-based report archive roots created by the QoS report pipeline SHALL keep only the most recent 100 timestamped report directories.
- Archive retention SHALL prune timestamped directories only and SHALL NOT treat helper symlinks such as `latest` as retention candidates.

## `scripts/install_nightly_full_regression_cron.sh`

- `scripts/install_nightly_full_regression_cron.sh` is the repo-local installer for the managed nightly cron entry.
- It SHALL install or refresh a single daily 03:00 cron line for `scripts/nightly_full_regression.py run`.
- Repeated installs SHALL be idempotent and SHALL NOT duplicate the managed entry.
