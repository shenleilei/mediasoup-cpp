# Design

## Context

The current accepted contract says:

- `scripts/run_all_tests.sh` is the full C++ regression entrypoint
- `scripts/run_qos_tests.sh` owns the broader QoS regression surface

That means the repository still has two different answers to "did we run everything?" The user requirement changes that: `run_all_tests.sh` must now be the one-command full regression entrypoint, including all accepted QoS tests.

## Goals

- make `scripts/run_all_tests.sh` cover the same QoS surface that `scripts/run_qos_tests.sh` already defines
- avoid maintaining two independent implementations of the QoS flow
- keep focused QoS and threaded-only reruns available
- rename the generated report so it no longer lies about being C++-only

## Proposed Approach

### 1. Delegate QoS coverage instead of duplicating it

Change `scripts/run_all_tests.sh` so its `qos` group runs:

- `scripts/run_qos_tests.sh all`

This makes `run_qos_tests.sh` remain the source of truth for:

- QoS JS tests
- QoS C++ tests
- plain-client QoS harnesses
- browser harnesses
- matrix runs
- QoS-specific reports and failure files

`run_all_tests.sh` will only own the top-level orchestration and summary reporting.

### 2. Keep `threaded` as a focused compatibility alias

`run_all_tests.sh threaded` should continue to exist, but it will delegate to:

- `scripts/run_qos_tests.sh cpp-threaded`

Because `qos` now includes the threaded QoS slice, group normalization in `run_all_tests.sh` will drop `threaded` whenever `qos` is also selected. This avoids duplicate execution during the default run while preserving the targeted CLI.

### 3. Expand the build stage for delegated QoS prerequisites

`scripts/run_qos_tests.sh` expects several repo-owned binaries to exist, including `mediasoup-sfu`.

Update `scripts/run_all_tests.sh`'s build stage to include:

- `mediasoup-sfu`

alongside the existing test binaries and `plain-client`. External runtime prerequisites such as browser tooling, Node modules, or netem remain outside the build graph and should still fail visibly if unavailable.

### 4. Rename the generated report

Once `run_all_tests.sh` covers more than C++ targets, `docs/full-cpp-test-results.md` becomes misleading.

Rename the generated report to:

- `docs/full-regression-test-results.md`

Update:

- `README.md`
- `docs/README.md`
- `docs/DEVELOPMENT.md`
- `specs/current/test-entrypoints.md`

## Group Model

- `unit`: existing native core unit regression
- `integration`: existing non-QoS integration/e2e/stability/review-fix regression
- `qos`: full QoS regression delegated to `scripts/run_qos_tests.sh all`
- `topology`: existing topology/multinode regression
- `threaded`: focused compatibility alias delegated to `scripts/run_qos_tests.sh cpp-threaded`

Normalization rule:

- if `qos` is selected, `threaded` is removed from the effective run list

## Failure Handling

- `run_all_tests.sh` keeps its continue-on-failure behavior across top-level groups
- if delegated QoS execution fails, the `qos` or `threaded` group is marked failed in the top-level report
- `run_qos_tests.sh` still writes its own failure file and QoS-specific reports

## Alternatives Considered

### Alternative: copy all QoS group logic into `run_all_tests.sh`

Rejected because it would create two separate implementations for the same QoS regression contract and immediately reintroduce drift risk.

### Alternative: add a brand-new third wrapper script for full regression

Rejected because the user explicitly wants `run_all_tests.sh` to be the full entrypoint, and another wrapper would preserve the current ambiguity.

### Alternative: keep the old report file name

Rejected because the file would again understate the actual entrypoint scope.

## Testing Strategy

- `bash -n scripts/run_all_tests.sh`
- `bash -n scripts/run_qos_tests.sh`
- `./scripts/run_all_tests.sh --list`
- `./scripts/run_qos_tests.sh --list`
- `./scripts/run_all_tests.sh --skip-build threaded`
- confirm a real run rewrites `docs/full-regression-test-results.md`
- confirm `--help` and `--list` do not rewrite the generated report

## Rollout Notes

- `scripts/run_qos_tests.sh` remains the focused QoS entrypoint for QoS-only work
- `scripts/run_all_tests.sh` becomes the top-level full regression entrypoint
