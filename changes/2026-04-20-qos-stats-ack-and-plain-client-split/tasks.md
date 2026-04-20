# Tasks

1. [x] Fix QoS stats response semantics
Outcome:
- `clientStats` / `downlinkClientStats` respond after worker processing and expose `stored/reason` truthfully
Verification:
- targeted `tests/test_qos_integration.cpp` cases

2. [x] Correct downlink rate-limit state around worker acceptance/rejection
Outcome:
- accepted sequence bookkeeping no longer advances speculatively before worker success
Verification:
- targeted downlink stale/rate-limit integration coverage

3. [x] Update accepted behavior and integration assertions
Outcome:
- specs/tests/docs describe stored-vs-ignored semantics without claiming synchronous planner completion
Verification:
- updated `specs/current/` plus targeted QoS tests

4. [x] Extract focused plain-client modules
Outcome:
- `client/main.cpp` no longer embeds the full WebSocket implementation and one additional helper area
Verification:
- `plain-client` target builds cleanly

5. [x] Run delivery verification and self-review
Outcome:
- targeted commands pass and remaining risk is documented truthfully
Verification:
- recorded command results and `REVIEW.md` pass
