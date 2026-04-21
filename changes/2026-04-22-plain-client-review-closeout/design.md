# Design

## Context

This is a narrow review-follow-up bugfix. The goal is to close the remaining correctness and verification debt without reopening the broader TWCC design.

The change intentionally stays inside the existing architecture:

- `SenderTransportController` remains the queueing / pacing boundary
- `NetworkThread` remains the transport owner
- `Prober` remains the background probe scheduler

## Fix 1: Make `SenderTransportController` Control Flow Obvious

The current indentation in several helper methods hides the true branch structure.

This change will:

- reindent `FlushAudioQueue()`
- reindent `FlushRetransmissionQueue()`
- reindent `FlushFreshVideoQueues()`
- reindent `SendPacket()`

No behavioral changes are intended in this step. The purpose is to remove a maintenance hazard that currently forces the reader to reconstruct control flow from braces instead of structure.

## Fix 2: Add Direct Audio Deadline Regression Coverage

The missing regression test should prove the local queue-expiry path directly instead of inferring it indirectly from wider bitrate scenarios.

The test shape will be:

1. configure `SenderTransportController` with a short `audioQueueDeadlineMs`
2. enqueue one audio packet
3. advance time past the deadline
4. call `OnPacingTick()`
5. assert:
   - the queue is empty
   - `audioDeadlineDrops` incremented
   - the audio packet was not counted as sent

This gives direct coverage for the review finding and protects the metric contract.

## Fix 3: Close Prober Listener Teardown Window

The review concern is not that the network thread forgets to stop the prober; it already calls `Reset()`.

The real issue is that `Prober` callbacks currently:

1. copy the raw `listener_` pointer under `mutex_`
2. release the lock
3. call through that pointer

That creates a teardown window where listener teardown can proceed after the pointer is copied but before callback completion.

### Chosen Approach

Add an internal callback-drain barrier to `Prober`:

- centralize listener callback dispatch through a helper
- track in-flight listener callbacks
- make listener reset / stop wait until those callbacks complete before returning

This keeps the current raw-pointer listener model, but upgrades it with explicit shutdown coordination.

### Why This Approach

- it fixes the actual race window inside `Prober`, not only the call site
- it does not require broader ownership refactors
- it preserves the existing external API shape
- it is small enough for a review follow-up change

## Spec Update

Update `specs/current/runtime-safety.md` to capture the accepted runtime-safety expectation for prober listener shutdown.

The behavior is operationally significant and belongs in accepted runtime-safety knowledge, not only in the change folder.

## Verification Strategy

- unit test suite containing `SenderTransportControllerTest`
- threaded integration suite covering probe / transport-cc behavior
- `git diff --check`
