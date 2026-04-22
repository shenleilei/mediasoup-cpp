# Bugfix Analysis

## Summary

Recent worker-side investigation surfaced a concern: some `PlainTransport C++ client` QoS logic may still rely on counter and stats assumptions that are only safe when all upstream layers expose identical semantics.

This change does not implement a fix yet. It records the code-level and principle-level findings in a durable repository document so future fixes can target the right boundaries.

## Reproduction

1. Trace plain-client sender QoS inputs from:
   - local RTCP state
   - server `getStats`
   - matrix/synthetic overrides
2. Compare how `packetsLost`, RTT, and counter deltas are normalized before they reach `deriveSignals`.
3. Observe where different sources are merged under one monotonic-counter assumption.

## Observed Behavior

- `packetsLost` semantics are not fully aligned across worker/server/plain-client boundaries.
- local RTCP and server stats are merged into the same QoS pipeline with only partial baseline normalization.
- both legacy and threaded plain-client paths carry similar loss-baseline logic, so any boundary mistake can exist twice.

## Expected Behavior

- the repository should have a clear, documented contract for sender-side QoS counters
- the plain-client QoS implementation should not rely on implicit counter semantics that vary by source
- maintainers should have one document that explains the highest-risk code/principle issues before making further QoS changes

## Known Scope

- `client/PlainClientSupport.cpp`
- `client/PlainClientSupport.h`
- `client/PlainClientLegacy.cpp`
- `client/PlainClientThreaded.cpp`
- `client/qos/QosSignals.h`
- `client/qos/QosTypes.h`
- `src/client/lib/qos/signals.js`
- `src/client/lib/qos/adapters/statsProvider.js`
- `client/RtcpHandler.h`
- relevant QoS docs under `docs/qos/plain-client/`

## Must Not Regress

- current accepted plain-client uplink QoS scope and status docs
- existing matrix/harness result entrypoints
- current worker-upgrade assessment docs and change records

## Suspected Root Cause

High confidence:

- sender QoS evolved around practical monotonic counters
- worker/server/client boundaries now expose enough nuance that the original assumptions are no longer obviously safe
- source switching and mixed counter origins were mitigated incrementally rather than unified under one explicit contract

## Acceptance Criteria

### Requirement 1

The repository SHALL contain a durable code/principle review document for plain-client QoS boundary risks.

#### Scenario: maintainer evaluates a sender-side QoS change

- WHEN a maintainer opens the plain-client QoS docs
- THEN they can find a document that summarizes the main code-level boundary risks
- AND the document does not depend on chat history

### Requirement 2

The review document SHALL identify the highest-confidence risks first.

#### Scenario: maintainer needs a prioritized reading of the problem

- WHEN the maintainer reads the document
- THEN it clearly separates confirmed contract mismatches from lower-confidence audit targets
- AND it points to the relevant files for each issue

## Regression Expectations

- Existing unaffected behavior:
  - no runtime code changes in this documentation step
- Required automated regression coverage:
  - `git diff --check` for changed docs
- Required manual smoke checks:
  - read the new document from the maintainer perspective
  - confirm discoverability from `docs/qos/plain-client/README.md`
