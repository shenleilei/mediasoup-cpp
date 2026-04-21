# Tasks

## Documentation Tasks

1. [ ] Add consolidated TWCC branch-delta summary
   Outcome:
   - A new doc under `docs/` explains the TWCC-related delta against `origin/codex/2026-04-20-functional-refactors`
   - The doc covers runtime, protocol, verification, and reporting changes
   Main files:
   - `docs/plain-client-twcc-change-summary.md`
   Verification:
   - Manual diff review against `git log origin/codex/2026-04-20-functional-refactors..HEAD`

2. [ ] Refresh architecture docs for the current threaded plain-client TWCC path
   Outcome:
   - The Linux client architecture doc and high-level architecture summaries describe the current `NetworkThread` / `SenderTransportController` / send-side BWE path
   Main files:
   - `docs/linux-client-architecture_cn.md`
   - `docs/architecture_cn.md`
   - `docs/full-architecture-flow_cn.md`
   Verification:
   - Manual review against current `client/PlainClientApp.*`, `client/PlainClientThreaded.cpp`, `client/NetworkThread.h`, and `specs/current/plain-client-send-side-bwe.md`

3. [ ] Refresh status pages and doc entrypoints
   Outcome:
   - Plain-client and QoS status pages link to the TWCC summary doc and stable generated A/B report
   Main files:
   - `docs/plain-client-qos-status.md`
   - `docs/qos-status.md`
   Verification:
   - Manual link and wording review

4. [ ] Run documentation sanity checks
   Outcome:
   - The edited markdown files are clean and internally consistent
   Verification:
   - `git diff --check`
