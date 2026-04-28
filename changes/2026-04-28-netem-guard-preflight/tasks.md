# Tasks

## 1. Guard Introspection

- [x] 1.1 Add reusable netem guard inspection and sweep helpers.
  Files: `tests/qos_harness/netem_guard.mjs`
  Verification: netem guard unit tests

## 2. Preflight CLI

- [x] 2.1 Add a CLI entrypoint for guard preflight and optional force-clear.
  Files: `tests/qos_harness/preflight_netem_guards.mjs`
  Verification: netem guard unit tests and script integration

## 3. Script Integration

- [x] 3.1 Run loopback netem preflight before each netem-dependent QoS task.
  Files: `scripts/run_qos_tests.sh`
  Verification: targeted shell-path review and netem guard unit tests

## 4. Verification

- [x] 4.1 Run `node --test tests/qos_harness/test.netem_guard.mjs`.
  Result: `node --test tests/qos_harness/test.netem_guard.mjs tests/qos_harness/test.cpp_client_runner_trace.mjs` passed.
