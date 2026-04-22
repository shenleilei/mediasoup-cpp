# Bugfix Design

## Context

`scripts/run_all_tests.sh` is the top-level regression entrypoint, but it delegates QoS and threaded slices to `scripts/run_qos_tests.sh`.

The current prebuild step does not respect that split. It always builds a broad superset of targets, including binaries that are only needed by delegated QoS/threaded flows.

Separately, the threaded integration target is a known large translation unit and should be treated as a special-case low-parallelism build on small hosts.

## Root Cause

`build_targets()` is hard-coded to build:

- all core unit/integration/topology targets
- QoS/threaded-only native targets
- `plain-client`

without consulting `SELECTED_GROUPS`.

## Fix Strategy

Replace the fixed build list with a selected-group-aware build list.

### Native Targets

Build only the native binaries required by:

- `unit`
- `integration`
- `topology`

### Delegated Groups

Do not prebuild QoS/threaded-only binaries in `run_all_tests.sh`.

Instead:

- let `scripts/run_qos_tests.sh` continue to ensure and build its own required targets

### Vendored Worker Coverage

Add a first-class `worker` group to `run_all_tests.sh`.

The default `non-qos` alias should expand to include this group so the top-level regression entrypoint always covers some vendored worker cases.

Because the current vendored `3.14.6` worker baseline may contain unrelated failures in parts of the full upstream suite, the default group should run a stable subset focused on relevant regression areas:

- RTCP RR parsing
- RTP/NACK retransmission behavior

This satisfies the requirement to include worker test cases without making the default repository regression depend on unrelated legacy failures in the full upstream worker suite.

### Heavy Target Parallelism

Teach `scripts/run_qos_tests.sh` to special-case:

- `mediasoup_thread_integration_tests`

and build it with:

- `cmake --build ... -j1 --target mediasoup_thread_integration_tests`

while leaving the existing behavior unchanged for other targets.

Similarly, run the vendored worker test invocation under:

- `taskset -c 0`

so the worker-local build uses a single CPU core on small hosts.

This keeps responsibilities aligned:

- `run_all_tests.sh` builds only what it directly needs
- delegated QoS/threaded runs build what they need inside their own entrypoint

## Risk Assessment

- If a native target is omitted for a non-delegated group, that group will fail at preflight.
- If the default full-run path forgets a target, the regression entrypoint will become incomplete.

## Test Strategy

- syntax-check the script
- inspect the computed target list for:
  - `non-qos`
  - default full run
- ensure no QoS/threaded-only targets are included in `non-qos`
- inspect the build command used for `mediasoup_thread_integration_tests`
- inspect the vendored worker command and confirm it uses a stable tag subset plus constrained parallelism

## Observability

- print the narrowed target list before the `cmake --build` invocation so failures remain easy to diagnose

## Rollout Notes

- No runtime deployment impact
- Safe rollback: restore the prior fixed target list
