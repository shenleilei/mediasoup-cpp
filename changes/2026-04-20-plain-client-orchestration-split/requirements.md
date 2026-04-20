# Problem Statement

`client/main.cpp` still mixes too many responsibilities:

- argument and environment parsing
- FFmpeg bootstrap and encoder recreation policy
- WebSocket join / plainPublish / session setup
- QoS controller wiring
- threaded runtime orchestration
- legacy runtime orchestration
- shutdown and cleanup

Even after extracting `WsClient`, `PlainClientSupport`, `common/media`, and `common/ffmpeg`, the `main()` function remains a long process-style orchestrator that is expensive to review and hard to change safely.

## Business Goal

Refactor the plain-client entrypoint so `client/main.cpp` becomes a thin executable shim while runtime orchestration is split into focused plain-client modules.

## In Scope

- `client/main.cpp`
- new plain-client orchestration modules under `client/`
- `client/CMakeLists.txt`
- affected live docs

## Out Of Scope

- changing plain-client CLI behavior
- changing QoS behavior
- changing threaded or legacy runtime semantics as a primary goal
- further FFmpeg/media helper extraction beyond what is needed for the orchestration split

## Acceptance Criteria

1. `client/main.cpp` SHALL be a thin entrypoint that does not directly contain the full runtime orchestration.
2. Threaded and legacy runtime orchestration SHALL be implemented in separate focused modules.
3. Shared setup/teardown logic SHALL be centralized instead of duplicated across runtime branches.
4. Existing plain-client build and targeted regressions SHALL remain green.

## Non-Functional Requirements

- Preserve current behavior.
- Keep orchestration modules explicit and readable; avoid speculative abstractions.
- Prefer one clear owner for runtime state instead of scattering globals across files.
