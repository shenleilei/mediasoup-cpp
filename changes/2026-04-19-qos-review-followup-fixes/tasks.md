# Tasks

## 1. Reproduce

- [x] 1.1 Capture the affected `clientStats` handoff, legacy demo path, and
  downlink rendering/sampling gaps
  Verification: targeted code inspection and existing tests identify the broken
  paths

## 2. Fix

- [x] 2.1 Eliminate duplicate publisher `clientStats` parsing
  Files: `src/SignalingServerWs.cpp`, `src/RoomService.h`,
  `src/RoomServiceStats.cpp`
  Verification: targeted QoS C++ tests

- [x] 2.2 Harden demo/playback downlink rendering and legacy-mode behavior
  Files: `public/qos-demo.js`, `public/playback.html`
  Verification: targeted JS tests plus manual reasoning against the rendered
  paths

- [x] 2.3 Enrich browser-side local downlink debug sampling and use public
  transport stats APIs
  Files: `public/qos-demo.js`, `src/client/lib/qos/downlinkSampler.js`,
  related JS tests if needed
  Verification: targeted JS tests

## 3. Unit And Integration Coverage

- [x] 3.1 Update or add unit tests for corrected QoS boundary logic
  Verification: run targeted unit tests

- [x] 3.2 Keep cross-boundary QoS integration coverage green
  Verification: run targeted integration tests

## 4. Guard Against Regression

- [x] 4.1 Add regression coverage for any newly clarified edge cases
  Verification: rerun the targeted regression checks

## 5. Validate Adjacent Behavior

- [x] 5.1 Verify unaffected QoS behavior still works
  Verification: run nearby C++ and JS suites

## 6. Delivery Gates

- [x] 6.1 Run required build/test commands for touched areas
  Verification: record command results

- [x] 6.2 Review `DELIVERY_CHECKLIST.md`
  Verification: all applicable items are resolved

## 7. Review

- [x] 7.1 Run self-review using `REVIEW.md` from the whole-system perspective
  Verification: root cause, regression risk, and boundary impact have been
  reviewed

## 8. Knowledge Update

- [x] 8.1 Update `specs/current/` only if accepted behavior changes
  Verification: reviewed against the final implementation; no accepted behavior
  spec update was required for this bugfix batch
