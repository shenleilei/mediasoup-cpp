# Bugfix Analysis

## Summary

The QoS review surfaced a small cluster of follow-up defects around `clientStats`
handoff, demo downlink rendering, and browser-side debug/downlink sampling:

- `clientStats` publisher snapshots are parsed on the WebSocket thread and then
  parsed again on the worker thread
- demo legacy mode still attempts to upload a non-schema-compliant
  `clientStats` payload that the server rejects by design
- playback/demo downlink rendering trusts optional fields and can show
  `NaN%`/broken values when older recordings or partial browser stats omit them
- browser-side downlink debug sampling omits concealment and freeze-related
  fields that the UI is prepared to show
- the demo reads transport stats through mediasoup-client private internals
  instead of the public `Transport.getStats()` API

## Reproduction

1. Send a valid `clientStats` request and trace the payload through
   `SignalingServerWs` and `RoomService`.
2. Open the demo with `?qos=legacy` and observe repeated `clientStats` upload
   attempts that the server rejects.
3. Load a recording or browser stats snapshot where audio concealment/freeze
   fields are absent.
4. Observe downlink UI values render as `NaN%` or stay blank despite available
   browser stats.

## Observed Behavior

- every valid `clientStats` message pays the validation/parse cost twice
- legacy demo mode silently exercises a known-invalid server path
- downlink UI code assumes optional fields exist and does not guard older or
  partial payloads
- demo local debug stats under-report downlink quality signals
- demo stats collection depends on `_handler._pc`

## Expected Behavior

- `clientStats` SHALL be parsed once on the signaling thread and the parsed
  snapshot SHALL be handed to the worker thread
- legacy demo mode SHALL avoid sending invalid `clientStats` and SHALL explain
  that only local debug is available in that mode
- playback/demo downlink rendering SHALL tolerate missing optional fields
- browser-side debug sampling SHALL collect the concealment/freeze fields that
  the UI surfaces when browsers expose them
- demo stats collection SHALL use public mediasoup-client transport APIs

## Known Scope

- `src/SignalingServerWs.cpp`
- `src/RoomService.h`
- `src/RoomServiceStats.cpp`
- `public/qos-demo.js`
- `public/playback.html`
- targeted C++ and JS regression tests

## Must Not Regress

- strict server-side `clientStats` schema validation
- existing QoS aggregation and stale-seq handling
- downlink reporter payload shape used by the server-side downlink planner

## Suspected Root Cause

The follow-up issues come from two sources:

- early demo/debug code paths predate the stricter QoS schema and newer
  browser-side downlink reporting
- the worker handoff kept the raw JSON payload even after validation was
  already completed on the WebSocket thread

Confidence: high.

## Acceptance Criteria

### Requirement 1

The system SHALL avoid duplicate parsing for valid publisher `clientStats`
requests.

#### Scenario: Single-parse handoff

- WHEN a valid `clientStats` request reaches the signaling server
- THEN validation happens on the WebSocket thread
- AND the worker thread receives the parsed snapshot rather than raw JSON

### Requirement 2

The demo SHALL degrade gracefully when legacy mode or older downlink payloads do
not provide full schema/data coverage.

#### Scenario: Legacy demo mode

- WHEN the demo runs in `?qos=legacy`
- THEN it does not spam the server with invalid `clientStats`
- AND it tells the user that only local browser debug stats are available

#### Scenario: Partial downlink payloads

- WHEN playback/demo renders downlink audio/video metrics with missing optional
  fields
- THEN the UI shows fallback placeholders instead of `NaN%` or script errors

### Requirement 3

Browser-side debug sampling SHALL expose the downlink fields that the current UI
already knows how to display when the browser reports them.

#### Scenario: Concealment and freeze metrics available

- WHEN inbound audio/video stats expose concealment or freeze counters
- THEN local debug sampling captures them and the UI can render them safely

## Regression Expectations

- Existing unaffected behavior: invalid `clientStats` requests are still
  rejected server-side
- Required automated regression coverage:
  - targeted QoS registry/integration tests
  - targeted JS tests for downlink sampler/protocol behavior
- Required manual smoke checks:
  - legacy demo mode no longer logs repeated invalid `clientStats` upload
    failures
