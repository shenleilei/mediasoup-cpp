# Bugfix Design

## Context

Wave 6 is a structural extraction step.

The architecture is already committed to:

- one real threaded sender QoS runtime
- bootstrap/session ownership in `PlainClientApp`
- runtime ownership for sampling, coordination, and transport execution

## Root Cause

The earlier rewrites fixed behavior first, but the threaded runtime implementation remained embedded in `PlainClientApp::RunThreadedMode()`. That leaves:

- bootstrap state and runtime state mixed together
- lifecycle code difficult to review in isolation
- future runtime work coupled to the app shell

## Fix Strategy

### 1. Introduce a dedicated threaded runtime object

Add a `ThreadedQosRuntime` implementation that accepts a `PlainClientApp` reference and owns:

- queue creation
- `NetworkThread` lifecycle
- audio/source worker lifecycle
- per-track QoS controller creation
- runtime sampling loop
- coordination and `clientStats` upload

### 2. Shrink `RunThreadedMode()` to a thin entrypoint

`PlainClientApp::RunThreadedMode()` should only instantiate the runtime object and call `Run()`.

### 3. Move runtime-only counters off `PlainClientApp`

Runtime-only sequencing/state such as:

- peer snapshot sequence
- last sample timestamp

should live inside the runtime object instead of the app shell.

## Risk Assessment

- Extraction can accidentally change startup ordering between network/audio/source/controller setup.
- Because the runtime uses many private `PlainClientApp` fields, the extraction must preserve exact data flow and teardown ordering.

## Test Strategy

- build `plain-client`
- build `mediasoup_qos_unit_tests`
- build `mediasoup_thread_integration_tests`
- run targeted sender QoS unit tests
- run representative threaded harness scenarios:
  - `publish_snapshot`
  - `threaded_generation_switch`
  - `threaded_multi_video_budget`

## Rollout Notes

- This wave is intended as a structural cleanup, not a behavior redesign.
- If behavior changes are discovered during extraction, treat them as regressions and fix them inside the new runtime object rather than restoring logic to `PlainClientApp`.
