# Tasks

## 1. Reproduce

- [ ] 1.1 Add or capture a failing test or reproducible check
  Verification: demonstrate the failure before the fix

## 2. Fix

- [ ] 2.1 Implement the minimal code change that removes the root cause
  Files: list the main modules
  Verification: rerun the failing test

## 3. Unit And Integration Coverage

- [ ] 3.1 Add or update unit tests for the corrected logic
  Verification: run the targeted unit tests

- [ ] 3.2 Add or update integration coverage if the defect crosses boundaries
  Verification: run the targeted integration tests

## 4. Guard Against Regression

- [ ] 4.1 Add regression coverage for the fixed path
  Verification: run the targeted regression checks

## 5. Validate Adjacent Behavior

- [ ] 5.1 Verify unaffected behavior still works
  Verification: run nearby unit or integration tests

## 6. Delivery Gates

- [ ] 6.1 Run lint, type-check, build, and required test suites
  Verification: record command results

- [ ] 6.2 Review `DELIVERY_CHECKLIST.md`
  Verification: all applicable items are resolved

## 7. Review

- [ ] 7.1 Run self-review using `REVIEW.md` from the whole-system perspective
  Verification: root cause, regression risk, and boundary impact have been reviewed

- [ ] 7.2 If the work is prepared as a branch or PR, review the full change set against the base branch
  Verification: global branch impact, regression risk, and missing updates are recorded

## 8. Knowledge Update

- [ ] 8.1 Update `specs/current/` if accepted behavior changed
  Verification: review docs against final behavior
