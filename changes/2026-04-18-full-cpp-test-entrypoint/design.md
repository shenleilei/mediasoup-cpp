# Design

## Context

`scripts/run_all_tests.sh` is currently documented and implemented as a non-QoS regression entrypoint. It excludes several suites from `mediasoup_tests`, skips the dedicated QoS binaries, and writes `docs/non-qos-test-results.md`.

That creates two practical problems:

- one command does not cover the full C++ regression surface
- some tests fall through the gap entirely because `run_all_tests.sh` excludes the whole QoS integration binary while `run_qos_tests.sh` only runs a focused subset of that binary

## Coverage Gap Summary

Current script behavior leaves uncovered cases in two ways:

1. `run_all_tests.sh` excludes QoS-oriented suites inside `mediasoup_tests` via a negative `gtest_filter`
2. `run_qos_tests.sh` intentionally narrows `mediasoup_qos_integration_tests` to a focused subset, so tests in the same binary that validate stats, recording, security denial, reconnect, and plain-publish behavior are not covered by either script

## Proposed Approach

Reframe `scripts/run_all_tests.sh` as the full C++ gtest regression entrypoint:

- remove the non-QoS-only framing from CLI help, status text, and docs
- split QoS unit sources out of `mediasoup_tests` into a dedicated `mediasoup_qos_unit_tests` binary
- run `mediasoup_tests` without the current exclusion filter because it now contains only the core unit slice
- add a dedicated `qos` group for the QoS C++ binaries:
  - `mediasoup_qos_unit_tests`
  - `mediasoup_qos_integration_tests`
  - `mediasoup_qos_accuracy_tests`
  - `mediasoup_qos_recording_accuracy_tests`
- keep existing `integration`, `topology`, and `threaded` groups for the non-unit binaries already owned by this script
- continue writing one Markdown result page after any actual execution, but rename that artifact to reflect the new full-C++ scope

Also tighten `scripts/run_qos_tests.sh`:

- `cpp-unit` runs `mediasoup_qos_unit_tests` directly
- `cpp-integration` runs `mediasoup_qos_integration_tests` directly
- remove the manually curated per-case whitelist filters for those dedicated QoS binaries
- split downlink output into:
  - `docs/downlink-qos-test-results-summary.md` for the always-written script summary
  - `docs/downlink-qos-case-results.md` for full downlink matrix case rendering

## Group Model

- `unit`: `mediasoup_tests`
- `integration`: `mediasoup_integration_tests`, `mediasoup_e2e_tests`, `mediasoup_stability_integration_tests`, `mediasoup_review_fix_tests`
- `qos`: `mediasoup_qos_unit_tests`, `mediasoup_qos_integration_tests`, `mediasoup_qos_accuracy_tests`, `mediasoup_qos_recording_accuracy_tests`
- `topology`: `mediasoup_topology_tests`, `mediasoup_multinode_tests`
- `threaded`: `mediasoup_thread_integration_tests`

This keeps partial runs understandable while making the default selection cover every C++ gtest target defined in `CMakeLists.txt`.

## Documentation And Report Naming

The old report name, `docs/non-qos-test-results.md`, becomes misleading after the scope change.

Rename the generated result page to `docs/full-cpp-test-results.md` and update references in:

- `README.md`
- `docs/README.md`
- `docs/DEVELOPMENT.md`
- `specs/current/test-entrypoints.md`

## Interfaces

- `scripts/run_all_tests.sh` keeps the same CLI pattern and continue-on-failure semantics
- `--list` now includes the added `qos` group
- the generated report path changes from `docs/non-qos-test-results.md` to `docs/full-cpp-test-results.md`
- `scripts/run_qos_tests.sh` keeps its existing group names, but its C++ groups stop exposing per-case whitelist behavior as part of the contract
- `scripts/run_qos_tests.sh` now writes the downlink summary to `docs/downlink-qos-test-results-summary.md`

## Failure Handling

- keep explicit preflight failure recording for missing binaries or missing commands
- keep the final aggregated failure summary and non-zero exit
- write the generated report before the final exit decision, as today

## Alternatives Considered

### Alternative: make `run_all_tests.sh` invoke `run_qos_tests.sh` in full

Rejected because that would fold browser, Node, client-JS, matrix, and other environment-heavy harness flows into the default native regression entrypoint. That materially changes dependencies, runtime, and flake surface far beyond the user-visible coverage hole in the current C++ regression flow.

### Alternative: keep QoS unit tests inside `mediasoup_tests` and widen the filter

Rejected because it still leaves the regression entrypoint dependent on naming conventions and hand-maintained filter expressions. Splitting the QoS unit sources into a dedicated binary makes future case additions inside those files script-transparent.

### Alternative: keep the old report file name

Rejected because the file name would be wrong after the entrypoint stops being non-QoS-only.

## Testing Strategy

- `bash -n scripts/run_all_tests.sh`
- `bash -n scripts/run_qos_tests.sh`
- `./scripts/run_all_tests.sh --list`
- `./scripts/run_all_tests.sh --skip-build unit`
- `./scripts/run_all_tests.sh --skip-build qos`
- `./scripts/run_qos_tests.sh --list`
- `./scripts/run_qos_tests.sh --skip-browser cpp-unit`
- `./scripts/run_qos_tests.sh --skip-browser cpp-integration`
- confirm the generated report is rewritten at `docs/full-cpp-test-results.md`
- confirm `--help` and `--list` do not rewrite the generated report

## Rollout Notes

- `scripts/run_qos_tests.sh` remains the focused QoS harness and matrix entrypoint
- the updated `run_all_tests.sh` becomes the one-command full C++ regression path
