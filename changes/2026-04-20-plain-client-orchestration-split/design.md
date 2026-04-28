# Design

## Goal

Make the executable entry thin and move orchestration into a dedicated plain-client runtime object.

## Proposed Structure

Add:

```text
client/
  PlainClientApp.h
  PlainClientApp.cpp
  PlainClientLegacy.cpp
  PlainClientThreaded.cpp
```

### Responsibilities

- `main.cpp`
  - call `RunPlainClientApp(argc, argv)`
- `PlainClientApp.cpp`
  - parse args/env
  - install signals
  - bootstrap FFmpeg/media state
  - connect/join/plainPublish/session setup
  - shared QoS setup
  - shared teardown
- `PlainClientLegacy.cpp`
  - legacy single-thread runtime loop
- `PlainClientThreaded.cpp`
  - threaded runtime orchestration

## State Ownership

Use one `PlainClientApp` object as the owner of:

- config
- media bootstrap state
- websocket/session state
- shared cached server stats state
- track runtime state

This keeps orchestration explicit without pushing more globals into the codebase.

## Testing Strategy

- `plain-client` standalone build
- root build targets affected by client code
- existing targeted shared-media and recorder regressions

## Risks

- moving large branches can introduce integration breakage if shared state is not grouped cleanly
- signal and shutdown behavior can regress if lifecycle ownership is split incorrectly

Mitigation:

- keep runtime behavior intact
- move logic mostly as-is, but into clearer owners/files
