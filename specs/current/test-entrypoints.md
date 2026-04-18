# Test Entrypoints

## `scripts/run_all_tests.sh`

- `scripts/run_all_tests.sh` is the full C++ gtest regression entrypoint.
- It SHALL continue running remaining selected groups after one selected group fails.
- It SHALL exit non-zero after the run if any selected group failed.
- It SHALL cover every C++ gtest target defined in `CMakeLists.txt`.
- After an actual test execution, it SHALL rewrite `docs/full-cpp-test-results.md`.
- The generated Markdown report SHALL include:
  - generation time
  - selected groups for that run
  - overall pass/fail status
  - failed-group summary
  - per-task status and elapsed time for each attempted task
- `--help` and `--list` SHALL exit without rewriting `docs/full-cpp-test-results.md`.

## `scripts/run_qos_tests.sh`

- `scripts/run_qos_tests.sh` is the QoS regression entrypoint.
- It SHALL continue running remaining selected groups after one selected task or group fails.
- Its C++ unit and C++ integration groups SHALL execute the dedicated QoS binaries without per-case whitelist filters.
- It SHALL record failed tasks in `tests/qos_harness/artifacts/last-failures.txt`.
- After an actual execution, it SHALL rewrite `docs/downlink-qos-test-results-summary.md`.
- When matrix-style groups run, it SHALL regenerate the corresponding QoS Markdown reports documented in `README.md`.
