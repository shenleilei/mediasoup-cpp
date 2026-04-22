# Bugfix Design

## Context

Wave 3 is the runtime cutover step.

The rewrite direction is already fixed:

- threaded path is the new sender QoS architecture
- legacy sender QoS runtime logic should not survive

## Root Cause

The runtime still allows sender QoS to be created in legacy mode because controller instantiation is not gated by `threadedMode_`, and the legacy loop still contains sender QoS code.

## Fix Strategy

### 1. Gate controller creation by threaded mode

Only create `PublisherQosController` instances when:

- `threadedMode_ == true`

### 2. Delete legacy sender QoS runtime logic

Remove from `RunLegacyMode()`:

- sender QoS sampling
- peer track state aggregation
- coordination override calculation
- client snapshot upload logic

Legacy mode remains a plain media send path only.

## Risk Assessment

- Any workflow that still expected sender QoS behavior from legacy mode will lose that behavior.
- If some logging or reporting depended on legacy sender QoS traces, those traces will disappear.

## Test Strategy

- build `plain-client`
- build `mediasoup_qos_unit_tests`
- run targeted QoS unit tests that still validate shared controller logic

## Rollout Notes

- This wave intentionally removes legacy sender QoS behavior.
- If sender QoS is needed, threaded mode is now the only supported runtime entry.
