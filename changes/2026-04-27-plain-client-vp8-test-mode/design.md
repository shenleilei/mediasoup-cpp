# Design

## Context
The repository now has two distinct truths:

1. Real browsers used manually can receive H264 from the plain-client.
2. The automated `headless_shell` environment used by browser black-box tests does not advertise H264 receive support, but does advertise VP8 and VP9.

If we insist on H264-only for the automated plain-client browser gate, the gate is invalid in this environment. To achieve reliable automated baseline interoperability coverage, we need a test-only codec path that the automated browser can actually consume.

## Design Choice
Add an explicit VP8 plain-client send mode while keeping H264 as the default.

This is intentionally a narrow compatibility mode for automated testing and does not replace the default manual H264 path.

## Proposed API Surface

### Plain-client runtime
Add a runtime-selectable video codec mode:
- default: H264
- explicit test mode: VP8

The selection mechanism should be simple and automation-friendly. Preferred approach:
- environment variable or explicit CLI flag scoped to the plain-client process

The automation harness will enable VP8 mode explicitly. Manual users get the current H264 behavior unchanged.

## Plain Publish Contract
The current `plainPublish` implementation is biased toward H264 baseline.

To support automated VP8 mode:
- the request path must be able to decide whether the plain publisher is H264 or VP8
- server-side producer RTP parameters must match the selected codec
- the returned publish metadata must include the actual payload type chosen for the codec

The smallest clean change is:
- extend `plainPublish` request data with an optional codec selector
- keep default codec = H264 for backward compatibility
- when selector = VP8, choose router VP8 capabilities instead of H264 baseline

## Plain-client Media Path

### H264 mode
No behavior change.

### VP8 mode
Required capabilities:
- encode or otherwise produce VP8 frames
- packetize RTP correctly for VP8
- integrate with existing RTCP / NACK / PLI behavior enough for the interop test

Because the input fixtures are H264 MP4 files, the plain-client VP8 mode will need a decode -> VP8 encode path. That is acceptable for automated testing because:
- this mode is explicit
- determinism matters more than raw efficiency

The simplest acceptable first implementation is:
- allow re-encode path for VP8 mode
- do not require copy mode in VP8 mode
- keep H264 copy mode untouched

## RTCP / Keyframe Handling
The VP8 path must preserve the minimum behavior needed for black-box rendering:
- packet store / retransmission remains intact
- PLI should trigger a new keyframe through the existing source/control path

Unlike H264, the current client code already assumes H264 packetization helpers. This area must be made codec-aware rather than globally H264-assumed.

## Black-box Test Design

### Test 1: `web -> web`
Use the real public demo page served by the SFU.
Pass criteria:
- publisher joins and publishes
- subscriber joins
- subscriber renders a non-zero-dimension remote video frame

### Test 2: `plain-client -> web`
Use the real public demo page plus the native plain-client in explicit VP8 mode.
Pass criteria:
- plain-client joins and publishes
- subscriber joins the same room
- subscriber renders a non-zero-dimension remote video frame

Both tests should:
- capture page console logs
- capture page errors
- capture recent SFU logs
- fail with actionable context

## Regression Entry Integration
The interop black-box target belongs in the browser harness group because:
- it validates real page rendering
- it is an end-to-end interoperability check

`scripts/run_qos_tests.sh` should run it early in `run_browser_harness()`, before more specialized downlink/QoS experiments.

Because `scripts/run_all_tests.sh` delegates the full QoS/browser surface through `scripts/run_qos_tests.sh`, this is sufficient to bring the interop gate under `run_all_tests.sh`.

## Risks

### Risk 1: Codec-aware plain-client refactor touches shared send paths
Mitigation:
- keep behavior split explicit and minimal
- preserve H264 default path
- add focused verification for both modes

### Risk 2: Test-only VP8 mode leaks into manual expectations
Mitigation:
- default remains H264
- docs clearly label VP8 as explicit interop test mode

### Risk 3: Black-box failure diagnostics are weak
Mitigation:
- standardize on page logs + server logs + plain-client logs in failures

## Verification Plan
- build `plain-client`
- run new browser interop harness directly
- run `scripts/run_qos_tests.sh browser-harness:public-interop`
- run the relevant `scripts/run_all_tests.sh` group that exercises the delegated QoS/browser path

## Design Summary
To make video interoperability the highest-priority automated requirement in the current environment:
- keep manual/default plain-client behavior unchanged
- add explicit VP8 test mode for plain-client
- make server plain publish honor that codec
- gate both `web -> web` and `plain-client -> web` through real black-box rendering tests
