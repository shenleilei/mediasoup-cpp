# Design

## Problem Framing
The current browser demo mixes two concerns:

1. baseline audio/video call behavior
2. experimental QoS-driven consumer control

That coupling is the design mistake.

The downlink planner currently interprets `visible`, `targetWidth`, `targetHeight`, and bandwidth-driven health signals as if they were stable immediately after subscription. They are not. Before the first decoded frame:

- the video element may still report `0x0`
- layout may not be final
- keyframe arrival may still be pending
- the subscriber may have just resumed or switched visibility

Yet the allocator can already classify a stream as low value and emit pause actions.

This is acceptable for an explicit stress / QoS experiment mode. It is not acceptable for the default demo path.

## Root Cause
There are three interacting root causes:

### 1. Wrong default operating mode
The browser demo enables active downlink QoS control by default instead of making it opt-in.

### 2. No bootstrap protection
The allocator and subscriber controller do not distinguish:

- "consumer exists but has not rendered yet"
- "consumer was visible and healthy, then later became expendable"

Those are not the same state and must not be governed by the same pause rules.

### 3. Overly aggressive pause semantics
The current logic treats low-value / not-visible streams as immediately pausable. This is especially dangerous when:

- the only visible remote video in a 1:1 session is still bootstrapping
- fake or extra cameras inflate perceived competition
- H264 late-join requires a fresh keyframe before decode can begin

## Goals
- Restore a stable, predictable default browser call path.
- Keep QoS experimentation available, but explicit.
- Prevent newly subscribed / resumed visible video from being paused before first-frame stabilization.
- Prevent the sole visible 1:1 main video from being auto-paused.
- Keep the design small enough to reason about and test.

## Non-goals
- Replacing the entire downlink planner.
- Eliminating all pause behavior from explicit QoS experiment mode.
- Solving every multi-party QoS optimization problem in this change.

## Proposed Operating Modes

### Mode A: Default Call Mode
Used by the ordinary browser demo without a QoS query flag.

Rules:
- no automatic downlink reporter uploads
- no active local QoS session that can suppress video
- no server-driven consumer pause behavior originating from browser demo downlink stats
- keyframe requests on new / resumed video consumers remain allowed

Expected outcome:
- The page behaves like a basic call demo first.

### Mode B: Explicit QoS Mode
Activated only by an explicit URL mode or future UI toggle.

Rules:
- downlink stats reporting enabled
- local QoS session enabled
- allocator may reduce layers / priority
- allocator may pause only after policy guardrails are satisfied

Expected outcome:
- The page behaves like a QoS experiment harness.

## Subscriber Lifecycle State Model

The allocator and controller should reason about subscriber video consumers using a lightweight lifecycle state model.

### State: `bootstrap`
Entered when:
- a new video consumer is created
- a paused consumer is resumed
- an invisible consumer becomes visible again
- the target render size transitions from zero to non-zero
- a consumer becomes the primary focused / pinned / speaker view

Properties:
- pause forbidden
- main-video sacrifice forbidden
- layer/priority reduction allowed
- immediate keyframe request sent
- delayed retry keyframe request allowed

### State: `stable`
Entered when all are true:
- first decoded frame observed
- minimum bootstrap duration elapsed
- several consecutive samples confirm stable visibility / dimensions

Properties:
- normal QoS rules apply

### State: `degraded`
Entered from stable when network health degrades.

Properties:
- reduce layers / priority first
- preserve 1:1 main video visibility
- only non-primary streams can become pause candidates

### State: `pausable`
Entered only if:
- stream is not the protected primary video
- stream has remained invisible or low-value for a hysteresis window
- bootstrap protection is over
- room shape makes sacrifice legitimate

Properties:
- pause becomes legal

## Bootstrap Protection Rules

### Enter Conditions
A video consumer enters protected bootstrap mode on:
- consumer creation
- consumer resume
- visible=false -> visible=true
- target size 0 -> non-zero
- promotion to main / pinned / active-speaker role

### Allowed Actions During Bootstrap
- request keyframe
- set priority
- reduce preferred layers

### Forbidden Actions During Bootstrap
- pause consumer
- convert the protected primary visible stream into an effectively absent video path

### Exit Conditions
Exit bootstrap only when all are true:
- first frame decoded or equivalent receiver evidence exists
- bootstrap minimum duration elapsed
- N consecutive healthy/visible samples observed

Recommended defaults:
- minimum bootstrap duration: 3 seconds
- stability window: 3 consecutive samples

### Hard Timeout
To avoid getting stuck forever:
- maximum bootstrap duration: 8 to 10 seconds

After hard timeout:
- primary 1:1 visible video remains non-pausable
- non-primary streams may re-enter normal QoS handling

## 1:1 Main Video Exemption

In a room where a subscriber has exactly one visible remote video that functions as the primary call video:

- automatic pause is disallowed
- degradation may only reduce layers / priority

This exemption should also apply to:
- pinned main view
- focused speaker view

This is the most important guardrail because it protects the fundamental call expectation.

## Visibility Hysteresis

The current semantic "not visible means pause candidate" is too sharp.

Replace it with:

1. `visible=false` lowers priority immediately
2. repeated invisibility over multiple windows is required before pause is allowed
3. pause eligibility also depends on stream role and room shape

Recommended defaults:
- invisibility hysteresis: 3 to 5 reporting intervals
- target size must remain zero or near-zero through the window

## Multi-camera / Fake Camera Policy

The demo currently attempts to publish all enumerated cameras. That is useful for stress experiments, but dangerous as a default because fake devices and extra cameras create artificial pressure.

Default browser demo policy:
- publish microphone plus the first camera only

Explicit experiment policy:
- multi-camera publishing enabled by mode or configuration

This removes artificial pressure from the default path while preserving experimental coverage when requested.

## H264-specific Handling

Late-join H264 subscribers are especially sensitive to keyframe timing.

Therefore:
- request a keyframe immediately on new video consumer creation
- request a second keyframe after a short delay if no first-frame evidence exists yet
- repeat the same pattern on resume

This does not replace bootstrap protection; it complements it.

## Observability

The system should log enough to explain every significant consumer-control decision.

At minimum, log:
- consumer enters bootstrap mode
- bootstrap exit reason
- pause rejected because bootstrap protection is active
- pause rejected because stream is protected primary video
- pause allowed after hysteresis
- keyframe requested on create/resume
- room-level mode: default call mode vs explicit QoS mode

Stats / state exposure should include:
- bootstrap flag
- bootstrap age
- first-frame-seen flag
- pause eligibility reason
- last pause/resume reason

## Rollout Strategy

### Phase 1
Restore default browser demo to basic call mode:
- no active QoS pause path
- single-camera publish default

### Phase 2
Add subscriber lifecycle protection in explicit QoS mode:
- bootstrap state
- 1:1 main-video exemption
- hysteresis before pause

### Phase 3
Re-enable more advanced QoS controls only after automated verification shows no first-frame regressions.

## Verification Plan

### Automated
- 1:1 browser demo: late join subscriber renders first frame
- 1:1 browser demo: no automatic pause of the only visible remote video
- resume path: resumed video requests keyframe and renders again
- explicit QoS mode: hidden non-primary streams can still be downgraded or paused after hysteresis
- multi-camera explicit mode: pause decisions only affect legitimate secondary streams

### Manual
- join publisher first, subscriber second
- join subscriber first, publisher second
- temporarily hide/show remote tile and confirm no immediate main-video pause
- pin/focus a video and confirm it remains protected

## Design Summary

The correct fix is not "remove pause forever."

The correct fix is:
- default call mode is non-experimental
- QoS control is opt-in
- pause is never allowed before bootstrap stabilization
- the primary 1:1 visible video is not a pause candidate
- multi-camera pressure is explicit, not accidental
