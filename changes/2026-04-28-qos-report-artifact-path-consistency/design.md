# Bugfix Design

## Context

QoS report generation spans shell orchestration, Node artifact helpers, markdown renderers, archives, and docs indexes. Path drift in any one layer makes generated outputs hard to discover and can silently break archive assumptions.

The current defect is concentrated in the plain-client targeted case report path, but the fix should also verify that the browser uplink and downlink path families are still internally consistent.

## Root Cause

- Shell and Node layers each carry artifact path literals.
- The plain-client helper module already treats the canonical targeted markdown as `docs/generated/plain-client-qos-case-results.targeted.md`.
- `run_qos_tests.sh` still writes targeted plain-client markdown to `docs/generated/uplink-qos-cpp-client-case-results.targeted.md`.
- Some docs indexes still label targeted outputs while linking full-report paths.

## Fix Strategy

### 1. Align shell output with helper-defined canonical path

- Update `run_qos_tests.sh` so targeted plain-client markdown is written to `docs/generated/plain-client-qos-case-results.targeted.md`.
- Keep the full markdown path unchanged at `docs/plain-client-qos-case-results.md`.

### 2. Add explicit regression coverage for plain-client artifact path helpers

- Extend the existing Node report-artifact tests with plain-client full/targeted path assertions.
- Verify default output inference for targeted plain-client matrix JSON.

### 3. Correct high-traffic docs indexes

- Fix `docs/README.md` targeted uplink/plain-client links so they point to targeted artifacts.
- Update the plain-client status/index doc that surfaces current artifact locations if it still only points at full markdown for targeted output.
- Leave deep historical analysis docs alone unless they are clearly serving as current artifact indexes; this keeps the bugfix narrow.

## Risk Assessment

- Changing the targeted markdown filename affects only future generated artifacts, not the already-archived historical snapshots.
- The largest risk is missing another consumer that hard-codes the old targeted filename; the audit and helper tests are intended to catch that.
- Doc fixes are low risk but should stay limited to current artifact index pages.

## Test Strategy

- Add or extend Node tests for `cpp_client_report_artifacts.mjs`.
- Run the Node artifact-path tests directly.
- Manually verify the relevant README/status links and the shell script output path literal.

## Observability

- No runtime logging changes are needed.
- The generated file layout itself is the observable outcome of this fix.

## Rollout Notes

- No migration is required.
- Historical archives with the old targeted filename may still exist and should remain untouched.
- The fix applies to future targeted report generations only.
