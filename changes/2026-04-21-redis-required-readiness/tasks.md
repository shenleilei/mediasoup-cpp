# Tasks

1. [in_progress] Define strict Redis-required runtime contract
Outcome:
- Change docs describe startup, readiness, metrics, and request-admission behavior with Redis as a strong dependency.
Verification:
- Docs in this change folder are internally consistent.

2. [pending] Implement startup and readiness enforcement
Outcome:
- Startup fails when Redis registry cannot initialize.
- Runtime snapshot, `/healthz`, `/readyz`, and metrics expose Redis requirement and readiness.
Verification:
- Targeted HTTP tests for health and readiness.

3. [pending] Remove implicit local-only fallback from request paths
Outcome:
- `join` and `/api/resolve` return explicit failure when Redis is unavailable.
Verification:
- Multinode integration tests cover Redis-unavailable request rejection.

4. [pending] Update accepted docs and monitoring guidance
Outcome:
- Durable docs/specs no longer describe implicit single-node fallback as default behavior.
Verification:
- Relevant `specs/current/` and ops docs updated.

5. [pending] Run targeted verification and self-review
Outcome:
- Tests and checks demonstrate the new Redis-required contract.
Verification:
- Build plus targeted test commands recorded in final handoff.
