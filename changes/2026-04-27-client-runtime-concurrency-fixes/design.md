# Design

## Context
These fixes are not feature work; they are correctness repairs in runtime plumbing that happens to be exercised by higher-level QoS and threaded client flows.

The goal is to remove fragile concurrency behavior without changing the supported external behavior of the main media/QoS paths.

## Root Cause

### 1. `Prober` lifecycle state is split across unsynchronized phases
`Run()` observes `stopRequested_` under `mutex_`, but `Reset()` writes `stopRequested_ = false` after releasing the lock and after joining the worker thread. At the same time `AddCluster()` can enqueue new work and make worker-thread startup decisions. That creates both:

- an actual data race on `stopRequested_`
- a stranded-work risk during reset if a cluster is queued while the old worker is still joinable

### 2. Probe send path mutates through a `const` API
`NetworkThread` finds a probe track through a const accessor and then increments `seq` via `const_cast`. Even if the backing object is currently mutable, the API contract is wrong and too easy to break later.

### 3. `WsClient` mixes an internal reader-thread model with a public receive API
The class already owns inbound WebSocket reads via `readerLoop()`, but still exposes `recvText()` publicly. That invites unsupported concurrent reads on the same fd. The fd lifecycle is also not fully serialized across:

- `sendText()`
- `recvText()` / `readerLoop()`
- `abortConnection()`
- `close()`

## Approach

### A. `Prober`: explicit synchronized worker lifecycle
Refactor `Reset()`/`Stop()`/`AddCluster()` so:

- worker-thread ownership can be moved out of the object under the lock
- lifecycle flags are only read/written under the mutex
- reset can safely re-enable the prober and restart the worker if clusters were queued during reset

This preserves the existing blocking `Reset()` semantics while making the worker lifecycle coherent.

### B. `NetworkThread`: mutable probe-track access
Introduce a non-const probe-track accessor for the send path and update `SendProbePacket()` to take a mutable track reference. Query-only helpers remain const.

### C. `WsClient`: single inbound owner + fd lifecycle guard
Make `recvText()` private and treat it as an internal primitive for:

- handshake completion before reader thread startup
- the reader thread itself

Then tighten fd lifecycle handling:

- serialize send/close interactions
- move fd detachment to a controlled critical section
- ensure repeated close/shutdown paths are idempotent
- avoid joining the reader thread while another thread can still race unsafely on the same fd state

## Non-goals
- Rewriting the plain-client signaling model wholesale.
- Changing higher-level QoS semantics.
- Adding speculative new abstractions beyond the minimal concurrency repair.

## Verification Strategy
- Add targeted unit coverage for `WsClient` close behavior where practical.
- Add targeted unit coverage for `Prober` reset/restart behavior where practical.
- Re-run the focused QoS regressions that depend on these runtime layers after the fixes land.
