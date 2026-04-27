# Tasks

1. [x] Repair `Prober` worker lifecycle synchronization.
   - Files: `client/ccutils/Prober.h`
   - Outcome: reset/stop/add-cluster cannot race on lifecycle flags or strand new clusters during reset.
   - Verify: targeted unit coverage if feasible and clean build

2. [x] Remove probe send `const_cast` mutation from `NetworkThread`.
   - Files: `client/NetworkThread.h`
   - Outcome: probe packets mutate sequence state only through non-const track access.
   - Verify: build and dependent targeted runtime checks

3. [x] Harden `WsClient` fd lifecycle and inbound ownership.
   - Files: `client/WsClient.h`, `client/WsClient.cpp`, relevant tests
   - Outcome: `recvText()` is internal-only and close/send/read interactions are synchronized safely.
   - Verify: `tests/test_ws_client.cpp` targeted coverage

4. [x] Re-run targeted verification and reconcile docs/tasks.
   - Files: affected change docs, any touched test/result docs
   - Outcome: concurrency fixes are verified and documented truthfully.
   - Verify: targeted unit tests plus impacted QoS/threaded regressions
