# Bugfix Plan: Downlink Auto-pause Guardrails

## Symptom
In the browser demo, a subscriber can join a room, receive `newConsumer` / `Subscribed video` successfully, and still fail to render the remote video because the downlink QoS path pauses the consumer too early.

## Reproduction
1. Open the browser demo in two tabs.
2. Join the same room from both tabs.
3. Publish audio/video from the sender tab.
4. Observe that the subscriber can show a paused consumer state before the remote video has rendered its first stable frame.

## Observed Behavior
- Signaling succeeds.
- Producer and consumer creation succeed.
- The subscriber may show `Subscribed video` while the video element never reaches a decoded frame.
- In some runs the consumer state is shown as paused.
- Environments that expose many camera devices or fake cameras amplify the issue because the QoS planner sees more streams and higher pressure.

## Expected Behavior
- The default demo path SHALL prioritize reliable 1:1 media rendering over automatic downlink pause behavior.
- A newly created or newly resumed visible video consumer SHALL NOT be auto-paused before it has a fair chance to render.
- The only visible remote main video in a 1:1 call SHALL NOT be auto-paused by default downlink QoS logic.
- Aggressive QoS controls SHALL be opt-in rather than the default browser demo behavior.

## Suspected Scope
- `public/qos-demo.js`
- `src/qos/DownlinkAllocator.cpp`
- `src/qos/SubscriberBudgetAllocator.cpp`
- `src/qos/SubscriberQosController.*`
- `src/RoomServiceDownlink.cpp`
- Related downlink QoS tests and browser harnesses

## Known Non-affected Behavior
- Basic signaling, transport creation, and consumer creation can succeed independently of this bug.
- Explicit QoS experimentation flows may still be desirable, but they should not be the default call path.

## Acceptance Criteria
- Default browser demo mode does not enable automatic consumer pause behavior.
- Downlink auto-pause is only active in an explicit QoS mode.
- Video consumers enter a protected bootstrap window after creation, resume, and visibility recovery.
- During the bootstrap window:
  - pause is forbidden
  - audio-only style sacrifice is forbidden for the protected main video
  - layer/priority reductions remain allowed
- A visible 1:1 main remote video is never auto-paused by the downlink allocator.
- Visibility loss alone does not immediately pause a consumer; hysteresis is required.
- Multi-camera / many-stream publisher scenarios are handled explicitly rather than implicitly through the default demo path.
- Sufficient logging and stats exist to explain why a consumer is paused, resumed, or protected.

## Regression Expectations
- A late-joining browser subscriber can render the first remote frame reliably.
- A resumed video consumer requests and receives a new keyframe path before pause is reconsidered.
- Existing explicit QoS test modes remain available, but their activation is deliberate.
