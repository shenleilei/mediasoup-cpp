# Tasks

1. [x] Freeze the target RoomRegistry architecture
Outcome:
- change docs define internal modules, ownership rules, and lock/thread rules explicitly
Verification:
- `requirements.md` and `design.md` align on module boundaries and invariants

2. [x] Introduce explicit internal cache and command owners
Outcome:
- cache and command connection are no longer implicit shared state piles
Verification:
- implementation follows the documented ownership split

3. [x] Isolate pub/sub worker lifecycle behind a dedicated internal module
Outcome:
- subscriber logic no longer lives as a free-floating set of methods mixed with claim/resolve concerns
Verification:
- subscriber behavior remains correct under reconnect and sync

4. [x] Refactor claim/resolve onto the new internal boundaries
Outcome:
- selection and Redis command flow become easier to reason about and review
Verification:
- targeted integration and multinode tests pass

5. [x] Re-run build, multinode, and operational regression checks
Outcome:
- the redesigned subsystem is verified against build, integration, and known operational pitfalls
Verification:
- required targets and checks pass
