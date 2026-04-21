# Plain-Client Review Closeout

## Symptom

The remaining review feedback for the plain-client sender transport / send-side BWE work is not fully closed:

- `SenderTransportController.h` still has misleading indentation in the queue-flush helpers and `SendPacket()`
- there is still no regression test that proves `audioDeadlineDrops` increments when queued audio expires
- `Prober` still allows a shutdown-time callback window where a listener pointer can be observed before teardown completes

## Reproduction

1. Read `client/SenderTransportController.h` around `FlushAudioQueue()`, `FlushRetransmissionQueue()`, `FlushFreshVideoQueues()`, and `SendPacket()`.
2. Observe that the indentation still obscures the real control flow.
3. Read `tests/test_thread_model.cpp` and observe there is no test that advances time beyond `audioQueueDeadlineMs` and asserts `audioDeadlineDrops`.
4. Read `client/ccutils/Prober.h` together with `client/NetworkThread.h` shutdown handling and observe that callbacks snapshot the raw `listener_` pointer without a teardown barrier.

## Observed Behavior

- The queue-flush code is technically correct but visually misleading.
- Audio expiry accounting is observable in production counters but not proven by a focused regression test.
- `NetworkThread::stop()` resets the prober before destruction, but `Prober` itself does not guarantee that in-flight callbacks have drained before listener teardown is complete.

## Expected Behavior

- Queue-flush helpers and `SendPacket()` should be formatted so their real control flow is obvious.
- Audio expiry should have a direct regression test that proves queue drop + metric increment behavior.
- Prober shutdown should not allow a listener callback to race with listener teardown.

## Suspected Scope

- `client/SenderTransportController.h`
- `client/ccutils/Prober.h`
- `client/NetworkThread.h`
- `tests/test_thread_model.cpp`
- `specs/current/runtime-safety.md`

## Known Non-Affected Behavior

- The actual `SenderTransportController` queue semantics appear functionally correct today.
- The transport-cc send-side BWE path, probe lifecycle, and existing harness/report entrypoints are already wired.

## Acceptance Criteria

1. The misleading indentation in `SenderTransportController.h` SHALL be corrected without changing the intended send/drop semantics.
2. A regression test SHALL verify that expired queued audio increments `audioDeadlineDrops` and removes the packet.
3. Prober shutdown SHALL explicitly prevent listener callbacks from outliving listener teardown.
4. Targeted unit and integration tests covering the touched behavior SHALL pass.

## Regression Expectations

- Existing `SenderTransportController` tests must continue to pass.
- Probe and threaded integration behavior must remain green after the lifecycle change.
