# Tasks

## 1. Planning Updates

- [ ] 1.1 Confirm open questions are resolved
  Verification: review requirements with stakeholders

- [ ] 1.2 Map acceptance criteria to verification activities
  Verification: every acceptance criterion has unit, integration, contract, or explicit manual coverage

## 2. Core Implementation

- [ ] 2.1 Implement the primary behavior
  Files: list the main modules
  Verification: code compiles and targeted checks are runnable

## 3. Unit Tests

- [ ] 3.1 Add or update unit tests for success, edge, and failure paths
  Verification: run the targeted unit test suite

## 4. Integration

- [ ] 4.1 Connect the new behavior to dependent modules
  Files: list the integration points
  Verification: run integration checks for affected boundaries

- [ ] 4.2 Add or update contract validation if APIs, events, schemas, or permissions changed
  Verification: run contract or integration verification

## 5. Hardening

- [ ] 5.1 Add edge-case handling, observability, and regression protection
  Verification: rerun test suite relevant to the change

## 6. Delivery Gates

- [ ] 6.1 Run lint, type-check, build, and required test suites
  Verification: record command results

- [ ] 6.2 Review `DELIVERY_CHECKLIST.md`
  Verification: all applicable items are resolved

## 7. Review

- [ ] 7.1 Run self-review using `REVIEW.md` from the whole-system perspective
  Verification: correctness, contracts, tests, docs, and operational impact have been reviewed

- [ ] 7.2 If the work is prepared as a branch or PR, review the full change set against the base branch
  Verification: global branch impact, missing updates, and regression risks are recorded

## 8. Knowledge Update

- [ ] 8.1 Update `specs/current/` if behavior is accepted
  Verification: review docs against final behavior
