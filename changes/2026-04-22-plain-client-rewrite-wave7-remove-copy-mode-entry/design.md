# Bugfix Design

## Context

Wave 7 removes the last legacy runtime entry from the main binary.

The architecture is already committed to:

- one real threaded plain-client runtime
- no compatibility baggage inside the main sender QoS product path
- failure-fast startup over silent fallback to older behavior

## Root Cause

The previous waves fixed control-path semantics first, but the main binary still carried:

- `copyMode_`
- `RunCopyMode()`
- fallback-to-copy startup behavior
- legacy-only codec/bootstrap state

That keeps two incompatible product surfaces in one executable.

## Fix Strategy

### 1. Remove copy-mode CLI/runtime state from `PlainClientApp`

- drop `copyMode_`
- remove `RunCopyMode()`
- make `Run()` always enter the threaded runtime

### 2. Remove copy-mode-only bootstrap/codec state

Delete fields and helpers that only existed for the legacy path, including:

- legacy packetization helpers
- copy-mode decode/filter state
- copy-mode fallback behavior

Retain only the bootstrap state still needed by the threaded runtime, such as:

- MP4/audio discovery when explicit sources are not provided
- initial FPS discovery

### 3. Remove legacy file from the main build

Stop linking `PlainClientLegacy.cpp` into `plain-client`.

## Risk Assessment

- Any local scripts still passing `--copy` will fail until updated.
- Bootstrap code must continue to support explicit source paths and file-backed threaded sources after removing the old decoder fallback path.

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

- If a minimal non-QoS sender is needed later, it should return as a separate binary instead of a mode flag on `plain-client`.
