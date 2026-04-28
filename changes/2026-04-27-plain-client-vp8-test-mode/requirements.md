# Requirements

## Problem Statement
The repository must treat end-to-end video interoperability as a highest-priority baseline requirement. The current automated browser environment used by black-box tests does not advertise H264 receive support, while the native PlainTransport C++ client currently sends H264 only. As a result, the `plain-client -> web` black-box path cannot be made a reliable automated gate in the current environment.

## Business Goal
Enable a deterministic automated interoperability test path so that:
- `web -> web` real rendering is gate-kept
- `plain-client -> web` real rendering is gate-kept
- both are covered by `run_all_tests.sh`

## In Scope
- Add an explicit VP8 send mode to the native plain-client.
- Allow automated tests to select VP8 without changing the default manual/demo plain-client behavior.
- Extend the plain publish signaling path so the server can create matching VP8 producers for this mode.
- Add black-box browser coverage that renders:
  - browser publisher -> browser subscriber
  - plain-client publisher -> browser subscriber
- Wire the black-box browser interop target into the existing QoS/browser regression entry path used by `run_all_tests.sh`.

## Out Of Scope
- Replacing the default plain-client H264 path for manual usage.
- Reworking all plain-client QoS logic or all codecs in one change.
- Supporting every codec combination in the first step.

## User Stories / Scenarios
- As a maintainer, I can run `scripts/run_all_tests.sh` and know whether browser-to-browser video rendering still works.
- As a maintainer, I can run `scripts/run_all_tests.sh` and know whether plain-client-to-browser video rendering still works in the repository’s supported automated environment.
- As a developer manually testing the plain-client, I can continue to use the existing H264 default path without changing my commands.

## Acceptance Criteria
- The plain-client SHALL continue to default to its current H264 behavior unless an explicit VP8 test mode is selected.
- The plain-client SHALL support an explicit VP8 mode that the automated harness can enable.
- The server plain publish path SHALL create a VP8 producer when the plain-client VP8 mode is used.
- A black-box browser test SHALL verify that a browser subscriber renders a first frame from a browser publisher using the real public demo page.
- A black-box browser test SHALL verify that a browser subscriber renders a first frame from a plain-client publisher using the real public demo page and the plain-client VP8 mode.
- The new black-box interop test target SHALL be invokable from `scripts/run_qos_tests.sh`.
- The `qos` group reached through `scripts/run_all_tests.sh` SHALL execute the interop black-box target.

## Non-functional Requirements
- The default plain-client behavior must remain backward compatible for manual usage.
- The new VP8 test mode must be deterministic enough for CI.
- Black-box tests must fail with actionable diagnostics including page logs and recent server logs.

## Edge Cases And Failure Cases
- Browser environment advertises VP8 but not H264.
- Plain-client exits early before publish completes.
- Browser subscriber joins after the plain-client publish completes.
- Browser subscriber receives audio but not video.
- The public demo page accidentally re-enables QoS pause behavior and hides the underlying interop result.

## Open Questions
- None for initial implementation; the selected scope is explicit VP8 test mode only.
