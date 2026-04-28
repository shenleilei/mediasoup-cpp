# Bugfix Design

## Context

The current netem locking model is correct for serialization, but operationally poor for repeated local runs because stale or conflicting locks are only discovered deep inside acquisition.

## Root Cause

- `acquireNetemGuard()` removes stale locks only after colliding with an existing directory
- callers do not have a preflight path to inspect or sweep guards
- `run_qos_tests.sh` launches netem-dependent tasks without checking current guard ownership first

## Fix Strategy

### 1. Add reusable inspection/sweep helpers

- Extend `netem_guard.mjs` with reusable guard-state inspection and sweep helpers.
- Report:
  - iface
  - owner metadata
  - stale-by-age / stale-by-pid
- Support:
  - clear stale guards
  - optionally clear live guards only when explicitly requested

### 2. Add a small CLI preflight entrypoint

- Add `preflight_netem_guards.mjs`.
- Behavior:
  - sweep stale guards
  - print what was cleared
  - fail immediately if live guards remain
  - optionally force-clear live guards when `--force-clear-live` is requested

### 3. Wire preflight into loopback-netem task launch

- Add a shell helper in `run_qos_tests.sh`.
- Invoke it before each loopback-netem-dependent task launch:
  - cpp-client matrix
  - cpp-client harness scenarios
  - browser harness scenarios
  - browser matrix
  - downlink matrix
- Keep the default behavior safe by not force-clearing live guards automatically.

## Risk Assessment

- Low risk for default mode: it only clears stale locks and improves diagnostics.
- Force-clear mode is intentionally destructive to conflicting runs, so it stays opt-in via environment variable.

## Verification

- Extend `test.netem_guard.mjs` with sweep behavior tests.
- Run the netem-guard tests directly.
