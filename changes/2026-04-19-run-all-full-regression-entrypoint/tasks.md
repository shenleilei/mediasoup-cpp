# Tasks

- [x] 1. Capture the new `run_all_tests.sh` scope in change docs
  - Files: `changes/2026-04-19-run-all-full-regression-entrypoint/requirements.md`, `changes/2026-04-19-run-all-full-regression-entrypoint/design.md`, `changes/2026-04-19-run-all-full-regression-entrypoint/tasks.md`
  - Verification: change docs explicitly describe `run_all_tests.sh` as the full regression entrypoint and define QoS delegation behavior

- [x] 2. Update `scripts/run_all_tests.sh` to delegate full QoS coverage
  - Files: `scripts/run_all_tests.sh`
  - Verification: `bash -n scripts/run_all_tests.sh`, `./scripts/run_all_tests.sh --list`, `./scripts/run_all_tests.sh --skip-build threaded`

- [x] 3. Update accepted specs and workflow docs for the new entrypoint semantics
  - Files: `specs/current/test-entrypoints.md`, `README.md`, `docs/README.md`, `docs/DEVELOPMENT.md`
  - Verification: docs refer to full regression scope and the renamed report path

- [x] 4. Produce the renamed generated report through a real run
  - Files: `docs/full-regression-test-results.md`
  - Verification: a real `scripts/run_all_tests.sh` execution rewrites the new report path
