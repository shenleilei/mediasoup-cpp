# Review3 Follow-up

## Symptom

`review3.md` mixed together three different classes of findings:

- defects that are still present in the current code
- issues already weakened or invalidated by the `review2` runtime-safety work and the `WorkerThread` single-thread execution model
- broader design concerns that still need a narrower proof before they should drive code changes

The current tree still contains several high-confidence bugs in protocol correctness, process/channel safety, and defensive error handling.

## Reproduction

1. Review the current implementations in `src/RtpParametersFbs.h` and `src/supportedRtpCapabilities.h`.
2. Review the worker/channel lifecycle paths in `src/Worker.cpp`, `src/Channel.cpp`, and `src/WorkerThread.cpp`.
3. Review external-input parsing in `src/RoomRegistry.cpp` and `src/MainBootstrap.cpp`.
4. Review recorder timestamp/QoS access in `src/Recorder.h`.

## Observed Behavior

- `repaired-rtp-stream-id` is matched by the earlier `rtp-stream-id` substring rule.
- supported RTP capabilities reuse payload type `108` for two different codec entries.
- the child process performs `std::vector`, `std::to_string`, and `setenv()` work after `fork()` and before `execvp()`.
- worker spawn failure paths leak pipe file descriptors.
- `Channel::sendBytes()` can race `close()` on `producerFd_`.
- Redis reply parsing assumes reply element type and `str` presence without validating them first.
- invalid numeric CLI arguments can throw `std::stoi`/`std::stod` exceptions and terminate startup.
- recorder QoS snapshots read `firstRtpTime_` under a different mutex from the writer.
- some worker-thread syscall error paths are silently ignored.

## Expected Behavior

- RTP header-extension URI mapping MUST distinguish repaired stream IDs correctly.
- supported RTP capabilities MUST not advertise duplicate payload types for different codecs.
- worker spawning MUST avoid non-async-signal-safe work after `fork()` and MUST clean up partial resources on failure.
- channel send/close paths MUST serialize fd lifetime so concurrent close/send cannot write to a stale or reused fd.
- Redis and CLI parsing MUST reject malformed input without crashing.
- recorder timestamp/QoS bookkeeping MUST use a consistent synchronization strategy.
- worker-thread eventing syscalls MUST surface failure and stop silently entering a degraded state.

## Suspected Scope

- `src/RtpParametersFbs.h`
- `src/supportedRtpCapabilities.h`
- `src/Worker.cpp`
- `src/Channel.cpp`
- `src/WorkerThread.cpp`
- `src/RoomRegistry.cpp`
- `src/MainBootstrap.cpp`
- `src/Recorder.h`
- targeted RTP/unit/runtime regression tests

## Known Non-Affected Behavior

- `review2` fixes around malformed WebSocket input, room-registry ordering, worker/router shutdown synchronization, channel self-close, and recorder allocation guardrails remain in place.
- room/peer state mutation still runs under the `WorkerThread` event-loop affinity documented in `src/WorkerThread.h`; several concurrency concerns from `review3.md` are therefore not currently confirmed bugs.

## Acceptance Criteria

- repaired RTP stream IDs map to the correct FlatBuffers enum.
- supported RTP capabilities expose unique payload types for distinct advertised codecs.
- worker spawn no longer performs unsafe post-`fork()` setup and closes partially created fds on failure.
- channel send/close races are removed or explicitly guarded.
- Redis reply parsing and CLI numeric parsing fail safely on malformed input.
- recorder QoS timing reads/writes are synchronized consistently.
- unchecked runtime syscalls in the worker-thread event loop are handled or logged explicitly.
- the change docs record which `review3.md` items were confirmed, deferred, or rejected after source validation.

## Regression Expectations

- existing RTP and ORTC negotiation tests continue to pass.
- existing worker/channel/runtime safety tests continue to pass.
- malformed Redis replies and malformed CLI flags gain regression coverage where practical.
- deferred `review3.md` items stay documented as follow-up work rather than being silently dropped.
