# Design

## Context

`review3.md` is useful as a review ledger, but it is not an implementation-ready change definition. A direct "fix everything in review3" pass would be too broad and would mix confirmed bugs with lower-confidence design concerns.

This change narrows the follow-up scope to defects that are still confirmed in the current tree after the `review2` fixes.

## Proposed Approach

### 1. Split validated defects from review debt

Keep a triage note in `implementation.md` that records, for each `review3.md` item, whether it is:

- confirmed in the current code
- not confirmed under the current architecture
- worth deeper follow-up but not yet implementation-ready

This prevents later work from re-litigating already-reviewed items.

### 2. Fix protocol and capability correctness first

Address the low-blast-radius correctness bugs first:

- repair the RTP header-extension URI matching order
- remove the duplicate advertised payload type in supported capabilities

These are deterministic bugs with straightforward regression tests.

### 3. Harden worker and channel lifecycle paths

For process/channel safety:

- move worker child argument/environment preparation out of the post-`fork()` window, or switch to a spawn path that avoids the unsafe child-side C++ setup
- close already-created pipe fds when later setup steps fail
- serialize channel write/close fd access so a send cannot race a close on `producerFd_`
- treat `epoll_ctl`, `eventfd`, and related loop syscalls as fallible runtime operations, not best-effort fire-and-forget calls

These fixes are coupled and should be reviewed together because they affect worker process lifecycle and degraded-state observability.

### 4. Harden malformed-input and timing paths

Apply narrow local fixes for:

- Redis reply type/null validation
- CLI numeric parse exception handling
- recorder QoS timestamp synchronization

These changes are local, testable, and lower-risk than the worker/channel work.

### 5. Keep weaker findings out of scope for this wave

Do not implement the weaker `review3.md` concerns in this change unless new evidence appears. In particular:

- `RoomService` `[this]` continuations are not currently confirmed as UAF under the worker-thread queue model
- `Peer` locking concerns are largely superseded by thread-affinity rules
- `GeoRouter` copy semantics are already implicitly disabled by its `std::mutex`
- `geo_` synchronization is startup-only today

These remain documented in the triage note so they can be revisited if the architecture changes.

## Alternatives Considered

### Fix every `review3.md` item in one branch

Rejected because it would mix deterministic bugs with speculative refactors and make verification weak.

### Ignore `review3.md` because some items are overstated

Rejected because several high-confidence defects are still present and should be tracked explicitly.

## Module Boundaries

- RTP serialization and capability declarations stay in `src/RtpParametersFbs.h` and `src/supportedRtpCapabilities.h`.
- process lifecycle stays in `src/Worker.cpp`, `src/Channel.cpp`, and `src/WorkerThread.cpp`.
- malformed external input handling stays in `src/RoomRegistry.cpp` and `src/MainBootstrap.cpp`.
- recorder synchronization stays local to `src/Recorder.h`.

## Failure Handling

- malformed Redis replies and malformed CLI flags should stay on an explicit error path instead of throwing or dereferencing null data
- worker/channel syscall failures should log actionable context instead of silently degrading runtime behavior
- worker spawn failure should leave no leaked fds behind

## Observability

- preserve or improve runtime logs on worker/channel failure paths
- reduce reliance on silent `catch (...)` behavior for newly touched paths

## Testing Strategy

- add or update targeted tests for RTP URI mapping and supported capability payload-type uniqueness
- add regression coverage for malformed Redis replies and invalid CLI numeric flags where repo structure makes that practical
- run targeted worker/channel/runtime suites after the lifecycle hardening work

## Rollout Notes

- protocol/capability fixes are low-risk and should land first if the work is split
- worker/channel lifecycle hardening is higher-risk and should be verified with targeted runtime tests before merge
