# Tasks

- [x] 1. Define the nightly automation contract in change docs
  - Files: `changes/2026-04-19-nightly-full-regression-email/requirements.md`, `changes/2026-04-19-nightly-full-regression-email/design.md`, `changes/2026-04-19-nightly-full-regression-email/tasks.md`
  - Verification: change docs describe scheduling, log retention, summary parsing, and email attachment behavior

- [x] 2. Implement the nightly full-regression wrapper
  - Files: `scripts/nightly_full_regression.py`
  - Verification: `python3 -m py_compile scripts/nightly_full_regression.py`

- [x] 3. Add runtime config template and cron installation flow
  - Files: `.nightly-full-regression.env.example`, `scripts/install_nightly_full_regression_cron.sh`, `.gitignore`
  - Verification: `bash -n scripts/install_nightly_full_regression_cron.sh`

- [x] 4. Update accepted specs and workflow docs
  - Files: `specs/current/test-entrypoints.md`, `README.md`, `docs/README.md`, `docs/DEVELOPMENT.md`
  - Verification: docs describe the nightly wrapper, log location, and scheduler install path

- [x] 5. Verify behavior with safe local checks
  - Files: generated runtime artifacts if produced during verification
  - Verification:
    - `python3 -m py_compile scripts/nightly_full_regression.py`
    - `bash -n scripts/install_nightly_full_regression_cron.sh`
    - targeted dry run of the nightly wrapper without launching the full regression
    - idempotent cron install check
