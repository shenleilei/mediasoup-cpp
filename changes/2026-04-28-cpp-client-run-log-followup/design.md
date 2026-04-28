# Bugfix Design

## Context

The `cpp-client-matrix` failures in the run-all log are not aligned with the emitted plain-client logs. The harness needs to follow the current logging contract, not the older raw-stdout assumption.

## Root Cause

There are two coupled bugs in `cpp_client_runner.mjs`:

1. trace rebuilding only reads `clientStdout`
2. `parseQosTraceLine()` requires the line to begin with `[QOS_TRACE]`

After the migration to `spdlog`, valid lines now typically arrive on stderr and include a prefix before the trace marker.

## Fix Strategy

- Add a helper that builds client trace state from both stdout and stderr.
- Update `waitForClientTrace()` to rely on the merged trace cache.
- Relax `parseQosTraceLine()` to locate `[QOS_TRACE]` anywhere in the line and parse the suffix from that marker onward.
- Add a focused Node test that covers:
  - stderr-only trace detection
  - spdlog-prefixed trace parsing

## Risk Assessment

- Low risk. The change is localized to harness parsing and does not alter runtime behavior of the SFU or plain-client.

## Verification

- Run the new targeted Node test.
- Re-run the previously implicated threaded integration subtests to confirm the old log’s separate hang point is not still present on current code.
