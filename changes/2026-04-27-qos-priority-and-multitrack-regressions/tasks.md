# Tasks

1. [x] Harden downlink startup budget handling.
   - Files: `src/qos/SubscriberBudgetAllocator.*`, relevant tests under `tests/`
   - Outcome: startup `availableIncomingBitrate=0` no longer forces visible consumers into immediate pause without corroborating congestion evidence.
   - Verify: targeted allocator tests and `scripts/run_qos_tests.sh browser-harness:downlink-priority`

2. [x] Reframe weighted multi-track verification onto the supported threaded/runtime path.
   - Files: `tests/qos_harness/run_cpp_client_harness.mjs`
   - Outcome: the regression checks validate threaded weighted budget behavior using synchronized local caps plus server-side target bitrate ordering instead of the misleading legacy last-sample proxy.
   - Verify: `node tests/qos_harness/run_cpp_client_harness.mjs threaded_multi_video_budget`

3. [x] Tighten multi-track regression assertions around supported weighted behavior.
   - Files: `tests/qos_harness/run_cpp_client_harness.mjs`, related tests/specs if needed
   - Outcome: regression checks validate trustworthy weighted ordering evidence instead of a misleading local-only proxy.
   - Verify: `scripts/run_qos_tests.sh cpp-client-harness:multi_video_budget`

4. [x] Re-run the focused QoS regression set and reconcile docs/specs.
   - Files: affected change docs and any accepted spec touched by the final supported contract
   - Outcome: both regressions are fixed with matching verification evidence and no doc drift.
   - Verify: focused reruns plus any necessary accepted-spec update review
