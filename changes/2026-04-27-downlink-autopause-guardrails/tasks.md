# Tasks

1. [ ] Split default browser demo mode from explicit QoS experiment mode.
   - Files: `public/qos-demo.js`
   - Outcome: default page no longer activates downlink / local QoS pause control automatically.
   - Verify: 1:1 browser join/publish/subscribe path renders video without pause state.

2. [ ] Add subscriber bootstrap protection state.
   - Files: `src/qos/SubscriberQosController.*`, `src/RoomServiceDownlink.cpp`, related helpers
   - Outcome: new/resumed visible video consumers cannot be paused before first-frame stabilization.
   - Verify: targeted unit/integration coverage for create/resume/bootstrap exit.

3. [ ] Add 1:1 main-video exemption and visibility hysteresis.
   - Files: `src/qos/DownlinkAllocator.cpp`, `src/qos/SubscriberBudgetAllocator.cpp`, supporting types
   - Outcome: sole visible remote main video cannot be auto-paused; hidden streams require repeated evidence before pause.
   - Verify: 1:1 and multi-stream allocator tests.

4. [ ] Make multi-camera publishing opt-in rather than default.
   - Files: `public/qos-demo.js`
   - Outcome: ordinary demo publishes microphone plus the first camera only; multi-camera remains available as an explicit mode.
   - Verify: browser demo publishes a single video track by default.

5. [ ] Improve H264 late-join keyframe handling.
   - Files: `src/RoomMediaHelpers.h`, `public/qos-demo.js`, related consumer-control paths
   - Outcome: new/resumed H264 video consumers reliably request fresh keyframes.
   - Verify: late-join H264 browser subscriber renders first frame.

6. [ ] Add observability for pause decisions and protection state.
   - Files: downlink QoS controller / stats paths / logs
   - Outcome: logs and stats explain why a consumer was paused, resumed, protected, or denied pause.
   - Verify: targeted log/state assertions in tests where practical.
