# Tasks

## 1. Define The Change

- [x] 1.1 Document the review-standard refresh requirements and design
  Files: `changes/2026-04-20-review-standard-refresh/requirements.md`, `changes/2026-04-20-review-standard-refresh/design.md`, `changes/2026-04-20-review-standard-refresh/tasks.md`
  Verification: change docs describe absorbed practices, non-goals, and verification

## 2. Refresh The Review Standard

- [x] 2.1 Strengthen `REVIEW.md` with reviewability, escalation, reviewer-coverage, and C++ backend verification rules
  Files: `docs/aicoding/REVIEW.md`
  Verification: review standard remains coherent and additive to the existing house style

## 3. Align Delivery And Prompt Layers

- [x] 3.1 Update the delivery checklist for the refreshed review rules
  Files: `docs/aicoding/DELIVERY_CHECKLIST.md`
  Verification: checklist reflects reviewability, design-level review, reviewer coverage, and high-risk verification expectations

- [x] 3.2 Update the reusable review prompt to match the source-of-truth review standard
  Files: `.github/prompts/review-whole-system.prompt.md`
  Verification: prompt asks for reviewability, missing expertise, and missing high-risk evidence

## 4. Verify

- [x] 4.1 Run documentation-focused verification
  Verification:
  - `git diff --check`
  - `git diff --stat`
  - `rg -n \"reviewability|qualified reviewer|explicit approval|high-risk\" docs/aicoding/REVIEW.md docs/aicoding/DELIVERY_CHECKLIST.md .github/prompts/review-whole-system.prompt.md`

- [x] 4.2 Run self-review using `docs/aicoding/REVIEW.md`
  Verification: final diff is internally consistent and does not introduce a second review standard
