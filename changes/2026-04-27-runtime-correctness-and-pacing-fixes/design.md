# Design

## Context
These fixes are mostly local correctness repairs, but they touch code that sits in hot or failure-prone runtime paths. The design priority is to make each fix explicit and testable without broad refactors unless required for safety.

## Approach

### 1. Producer pause/resume acknowledgement
Treat missing producers as explicit operational failures. This aligns behavior with the rest of the RoomService API and removes silent no-op success from signaling/QoS flows.

### 2. Legacy pacing byte budget
Preserve the existing non-transport-controller fallback path but make each pacing tick honor a byte budget. A simple budget proportional to the configured pacing interval is sufficient; the key is to prevent fixed-size bursts from bypassing pacing intent.

### 3. Deferred join response lifetime
Replace raw `WorkerThread*` deferred capture with a capture that is lifetime-safe under stop/shutdown ordering. The fix should avoid use-after-free without introducing circular ownership.

### 4. Fresh-video queue fairness
Advance the next-track cursor more consistently when the current track is paused or empty so the scheduler does not repeatedly begin on a dead slot.

### 5. netem stale lock hardening
Convert stale lock cleanup into a safer ownership check/removal sequence that minimizes deleting another process’s newly acquired lock.

## Non-goals
- Replacing the legacy pacing path with the transport-controller path.
- Re-architecting the whole signaling server lifetime model beyond what is needed to make deferred captures safe.

## Verification Strategy
- Add targeted unit coverage for recorder unwrap and missing producer pause/resume.
- Re-run targeted QoS/browser regressions impacted by pacing and signaling behavior.
- Re-run netem guard tests if the lock implementation changes.
