# Bugfix Analysis

## Summary

`/var/log/run_all_tests.log` shows the current `cpp-client-matrix` failures are harness-side false negatives rather than plain-client runtime failures:

- the harness waits for `QOS_TRACE` but only rebuilds trace state from client stdout
- plain-client now emits `QOS_TRACE` through `spdlog`, which places those lines on stderr
- the parser only accepts lines that start with `[QOS_TRACE]`, but current logs include the spdlog prefix before that marker

## Reproduction

1. Inspect `tests/qos_harness/cpp_client_runner.mjs`.
2. Note that `waitForClientTrace()` depends on `traceCache.trace.length > 0`.
3. Note that `traceCache` is rebuilt from `diagnostics.clientStdout` only.
4. Compare that with `/var/log/run_all_tests.log`, where the actual plain-client log lines are shaped like:
   `[timestamp] [info] [QOS_TRACE] ...`

## Observed Behavior

- `cpp-client-matrix` can report `plain-client did not emit QOS_TRACE in time` even when many `QOS_TRACE` lines were emitted.
- Once the first wait fails, the matrix run degrades into `errors=48 total=48`.

## Expected Behavior

- the cpp-client harness SHALL detect `QOS_TRACE` regardless of whether it arrives on stdout or stderr
- the parser SHALL accept both legacy bare `[QOS_TRACE] ...` lines and spdlog-prefixed lines containing `[QOS_TRACE]`

## Known Scope

- `tests/qos_harness/cpp_client_runner.mjs`
- new targeted Node regression coverage under `tests/qos_harness/`

## Acceptance Criteria

### Requirement 1

The harness SHALL build trace state from the actual plain-client log streams.

#### Scenario: trace only in stderr

- WHEN stderr contains a valid `QOS_TRACE` line and stdout contains none
- THEN the harness helper still resolves a non-empty trace

### Requirement 2

The harness SHALL parse spdlog-prefixed `QOS_TRACE` lines.

#### Scenario: spdlog-formatted trace line

- WHEN a line is shaped like `[timestamp] [info] [QOS_TRACE] ...`
- THEN it is parsed identically to the legacy bare `[QOS_TRACE] ...` format
