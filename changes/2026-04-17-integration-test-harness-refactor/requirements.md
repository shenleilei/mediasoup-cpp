# Requirements

## Summary

Refactor the C++ integration-test support code so repeated SFU process startup, shutdown, port waiting, unique test naming, and default RTP capability setup are centralized in shared test helpers.

## Business Goal

Developers need the integration-test layer to be easier to maintain across the repository so test changes do not require copying and updating the same process-control logic in many files.

## In Scope

- shared C++ test helper headers under `tests/`
- migration of representative single-node and multi-node integration test suites to the shared helpers
- consolidation of repeated default RTP capability builders where behavior is identical
- consolidation of repeated producer RTP parameter JSON builders where behavior is identical
- consolidation of repeated HTTP test helpers where behavior is identical
- consolidation of repeated WebSocket signaling request helpers where behavior is identical
- consolidation of repeated WebSocket connect-plus-join helpers where behavior is identical
- consolidation of repeated media test fixtures where behavior is identical

## Out Of Scope

- behavior changes in runtime server code
- changing test coverage intent or scenario assertions
- modifying Node.js QoS harness scripts
- changing build target names or which tests are registered

## User Stories

### Story 1

As a developer
I want SFU test process control in one place
So that startup and teardown changes apply consistently across integration suites

### Story 2

As a developer
I want common default RTP capability fixtures reused
So that similar tests do not drift in duplicated JSON literals

## Acceptance Criteria

### Requirement 1

The system SHALL centralize repeated SFU test process lifecycle logic into shared helpers.

#### Scenario: single-node integration suite startup

- WHEN a single-node integration test needs to start `mediasoup-sfu`
- THEN it SHALL use shared helper code for command assembly, startup wait, and shutdown wait
- AND the suite SHALL preserve its existing port and Redis settings

#### Scenario: multi-node integration suite startup

- WHEN a multi-node integration test needs to start more than one `mediasoup-sfu` process
- THEN it SHALL be able to reuse the same shared helper with per-node configuration overrides

### Requirement 2

The system SHALL preserve the existing integration test behavior and target names.

#### Scenario: migrated test suites

- WHEN the migrated tests are built and run after the refactor
- THEN their target names SHALL remain unchanged
- AND their scenario assertions SHALL not change

### Requirement 3

The system SHALL centralize identical default RTP capability fixtures where practical.

#### Scenario: standard audio+video WebRTC client fixture

- WHEN a suite uses the repository's standard Opus + VP8 capability fixture
- THEN it SHALL be able to source that fixture from shared helper code instead of duplicating the literal

### Requirement 4

The system SHALL centralize identical producer RTP parameter fixtures where practical.

#### Scenario: standard Opus or VP8 producer parameters

- WHEN a suite produces media with the repository's standard single-encoding Opus or VP8 RTP parameters
- THEN it SHALL be able to source those parameters from shared helper code instead of duplicating the JSON literal

### Requirement 5

The system SHALL centralize identical HTTP test helpers where practical.

#### Scenario: simple HTTP GET in integration tests

- WHEN a suite issues HTTP GET requests against the SFU for APIs or static files
- THEN it SHALL be able to use shared helper code for raw response and body extraction

### Requirement 6

The system SHALL centralize identical WebSocket signaling request helpers where practical.

#### Scenario: standard join and transport setup in integration tests

- WHEN a suite performs the repository's standard `join`, `createWebRtcTransport`, or `produce` request shapes
- THEN it SHALL be able to use shared helper code instead of repeating equivalent request payload assembly

### Requirement 7

The system SHALL centralize identical WebSocket connect-plus-join flows where practical.

#### Scenario: default integration-test peer session setup

- WHEN a suite needs a connected WebSocket client that immediately joins with a standard RTP capability fixture
- THEN it SHALL be able to use shared helper code for the connect-plus-join flow

### Requirement 8

The system SHALL centralize identical media test fixtures where practical.

#### Scenario: audio-opus loopback RTP test setup

- WHEN a suite creates an audio-only router fixture and loopback UDP socket for RTP/PlainTransport tests
- THEN it SHALL be able to use shared media helper code instead of duplicating the same codec and socket setup

## Non-Functional Requirements

- Maintainability: common test harness logic should be implemented once
- Reliability: shared startup and teardown behavior must remain deterministic
- Compatibility: migrated suites must keep their existing ports and runtime assumptions
- Observability: failures should still point clearly to startup, teardown, or assertion stages

## Edge Cases

- tests that need Redis-backed SFU instances must still be able to override Redis host and port
- tests that need different worker counts or `maxRoutersPerWorker` values must keep that control
- tests with custom RTP capability fixtures must not be forced onto the shared default fixture

## Open Questions

- None; runtime behavior is intentionally unchanged
