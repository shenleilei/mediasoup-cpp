# Downlink QoS

## Stored Subscriber Snapshots

- An accepted `downlinkClientStats` snapshot SHALL be visible through `getStats` as `downlinkClientStats`.
- For `downlinkClientStats`, "accepted" means the request response returned `ok=true` with `data.stored=true`.
- `downlinkClientStats.subscriptions` SHALL preserve valid consumer / producer mappings and SHALL drop inconsistent mappings before storage.

## Async Downlink Health

- `downlinkQos.health` and `downlinkQos.degradeLevel` are derived from worker-side downlink planning state.
- Callers SHALL treat `downlinkQos` as eventually consistent with a just-accepted `downlinkClientStats` request rather than as a synchronous request-response field.

## Tight-Budget Priority

- When one subscriber competes for limited incoming bitrate across a visible screen-share consumer and a visible grid consumer, the screen-share consumer SHALL NOT receive a lower allocation than the grid consumer.
- Under a budget that can admit only one base-layer video consumer, the screen-share consumer SHALL be admitted ahead of the visible grid consumer.
