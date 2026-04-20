# Tasks

1. [completed] Land the protocol-correctness fixes
Outcome:
- RTP header-extension URI mapping and supported capability payload types are corrected and regression-tested
Verification:
- targeted RTP/ORTC unit tests

2. [completed] Harden worker spawn and channel fd lifecycle
Outcome:
- post-`fork()` unsafe setup is removed or isolated, partial pipe creation is cleaned up, and channel send/close fd races are guarded
Verification:
- targeted worker/channel/runtime tests

3. [completed] Harden malformed-input and syscall error handling
Outcome:
- Redis reply parsing, CLI numeric parsing, and worker-thread runtime syscalls fail safely with explicit diagnostics
Verification:
- targeted malformed-input regressions and runtime tests

4. [completed] Fix the remaining local confirmed correctness items
Outcome:
- recorder QoS timestamp access, delayed-task move/copy behavior, header static data placement, and MIME suffix detection are cleaned up where still in scope
Verification:
- targeted unit/integration coverage for touched modules

5. [completed] Re-run review and record final disposition of review3 items
Outcome:
- confirmed fixes, deferred concerns, and rejected/invalid findings are documented truthfully in the change docs
Verification:
- `REVIEW.md` pass and updated `implementation.md`
