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
