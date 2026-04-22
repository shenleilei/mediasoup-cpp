# Design

## Approach

Add a new dedicated documentation file under `docs/qos/plain-client/` that covers both:

- runtime architecture
- thread model

The document will be written against the current code structure, especially:

- `PlainClientApp`
- `PlainClientThreaded`
- `ThreadedQosRuntime`
- `NetworkThread`
- `SourceWorker`
- `ThreadedControlHelpers`

## Structure

The document should include:

1. Purpose and scope
2. Top-level component map
3. Thread inventory table
4. Per-thread responsibilities
5. Queue / command / ack topology
6. Runtime startup flow
7. Runtime steady-state loop
8. QoS inner loop / outer loop boundary
9. Why one video worker per track and one network thread globally
10. Ownership and invariants

## Link Updates

Update the plain-client `README.md` and current architecture summary to point readers to the new single detailed document.

## Verification

- review for consistency with current code
- `git diff --check` on the changed docs
