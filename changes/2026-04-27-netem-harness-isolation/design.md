## Context

QoS harnesses currently mutate the global `lo` root qdisc directly. They rely on best-effort cleanup in individual scripts, but they do not coordinate ownership across harnesses and they do not reliably recover from interruption. Because `lo` root qdisc is machine-global state, a stale `netem` configuration contaminates every later loopback/browser harness.

## Goals

- Make `lo` root qdisc usage exclusive for QoS harnesses.
- Clear stale root qdisc state before a harness starts.
- Ensure common exit paths release both the lock and the root qdisc.
- Stop matrix runs early when baseline state is already contaminated.

## Non-Goals

- Rewriting all harness orchestration.
- Changing QoS decision thresholds or case expectations.
- Serializing non-netem test groups that do not touch `lo` qdisc.

## Approach

### 1. Shared netem guard utility

Add a shared Node utility for netem-based harnesses that:

- acquires an exclusive lock using a filesystem lock directory
- records owner metadata (`pid`, timestamp, label)
- removes stale locks when the recorded process is gone or the lock exceeds a bounded age
- clears `lo` root qdisc immediately after lock acquisition
- registers exit/signal handlers to clear qdisc and release the lock

This keeps coordination local to the harnesses that actually touch `tc`.

### 2. Integrate guard into all Node harnesses that mutate `lo`

Use the shared guard in:

- `loopback_runner.mjs`
- `cpp_client_runner.mjs`
- `browser_loopback.mjs`
- `browser_downlink_priority.mjs`

For reusable helpers such as `loopback_runner.mjs` and `cpp_client_runner.mjs`, the guard is acquired once per harness lifecycle and reused for all subsequent `applyNetemConfig()` / `clearNetem()` calls.

### 3. Shell-level cleanup in `run_qos_tests.sh`

Add a conservative shell helper that clears `tc qdisc dev lo root` before and after netem-based groups. This does not replace the lock, but it reduces contamination from interrupted prior runs before the Node harnesses even start.

### 4. Matrix fail-fast on contaminated baseline

Add a targeted baseline sanity guard in `run_matrix.mjs`:

- after the baseline phase completes
- before the case is evaluated normally

If the baseline phase is already at a clearly degraded state that is incompatible with the scenario's baseline network, treat it as an infrastructure failure and abort the matrix run instead of producing many misleading case failures.

The guard should stay narrow and conservative:

- it should trigger on obviously impossible baseline contamination
- it must not reject legitimate baseline cases such as `B3`

## Risks

- Lock logic can deadlock if stale lock detection is wrong.
- Over-aggressive baseline sanity checks could reject legitimate boundary scenarios.
- Signal cleanup cannot handle `SIGKILL`; stale-lock recovery and acquire-time qdisc cleanup are therefore required.

## Verification

- Add a small Node test for lock acquisition, stale lock recovery, and release cleanup behavior.
- Run targeted browser and matrix harness verification from a clean environment.
- Confirm `tc qdisc show dev lo` is clean after harness completion.
