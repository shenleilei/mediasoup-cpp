# Test Entrypoints

## `scripts/run_all_tests.sh`

- `scripts/run_all_tests.sh` is the non-QoS full-regression entrypoint.
- It SHALL continue running remaining selected groups after one selected group fails.
- It SHALL exit non-zero after the run if any selected group failed.
- After an actual test execution, it SHALL rewrite `docs/non-qos-test-results.md`.
- The generated Markdown report SHALL include:
  - generation time
  - selected groups for that run
  - overall pass/fail status
  - failed-group summary
  - per-task status and elapsed time for each attempted task
- `--help` and `--list` SHALL exit without rewriting `docs/non-qos-test-results.md`.

## `scripts/run_qos_tests.sh`

- `scripts/run_qos_tests.sh` is the QoS regression entrypoint.
- It SHALL continue running remaining selected groups after one selected task or group fails.
- It SHALL record failed tasks in `tests/qos_harness/artifacts/last-failures.txt`.
- When matrix-style groups run, it SHALL regenerate the corresponding QoS Markdown reports documented in `README.md`.
