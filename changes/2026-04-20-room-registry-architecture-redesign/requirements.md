# Problem Statement

`RoomRegistry` is now implemented across multiple files, but it still behaves like one large subsystem with weakly defined internal boundaries.

Today it combines at least five concerns:

- command connection lifecycle and reconnect behavior
- node/room cache ownership
- room claim / resolve public commands
- background pub/sub subscriber lifecycle
- Redis sync/scan/mget and node-selection policy

Even if these functions are split across `.cpp` files, the architecture is not yet defined to a high enough standard because:

- state ownership is implicit rather than enforced by module boundaries
- thread and lock rules are documented informally but not encoded in the design
- dependency direction between command I/O, cache mutation, pub/sub, and selection policy is not yet treated as a first-class contract

Before this subsystem goes live, it should be designed as a stable architecture instead of continuing as an organic collection of methods.

## Business Goal

Define a high-standard internal architecture for `RoomRegistry` so future implementation work is guided by explicit module boundaries, ownership rules, concurrency contracts, and verification gates.

## In Scope

- the internal architecture of `RoomRegistry`
- module boundaries inside `src/RoomRegistry*`
- ownership rules for:
  - Redis command connection
  - subscriber connection
  - cache state
  - worker thread handoff points
- lock and thread-affinity rules
- phased migration plan from the current implementation
- verification expectations

## Out Of Scope

- changing external Redis key formats
- changing room-claim or room-resolve product semantics as a primary goal
- adding new Redis features such as auth pools or sentinel support in the same change
- changing `RoomService` or `SignalingServer` public contracts unless required by the internal redesign

## Scenarios

### Scenario 1: Claim/resolve correctness review

A reviewer can identify exactly which internal module owns:

- command I/O
- cache mutation
- selection policy

without tracing unrelated subscriber or sync code.

### Scenario 2: Subscriber bugfix

A developer fixing subscriber reconnect or pub/sub parsing can work inside the subscriber module without touching room claim/resolve policy.

### Scenario 3: Cache and Redis lock safety

A developer can tell by design, not convention, which code is allowed to:

- hold `cmdMutex_`
- hold `cacheMutex_`
- touch both
- perform Redis I/O

## Acceptance Criteria

1. The redesign SHALL define explicit internal modules for command I/O, cache ownership, pub/sub, sync, and selection/claim policy.
2. The redesign SHALL define which state each module owns and which state it may only observe.
3. The redesign SHALL define clear thread/lock rules that prevent Redis I/O under cache lock.
4. The redesign SHALL preserve a stable `RoomRegistry` facade while narrowing internal responsibilities.
5. The redesign SHALL include a phased migration plan so implementation can proceed incrementally instead of as a one-shot rewrite.
6. The redesign SHALL include a verification matrix covering unit, integration, and operational regression checks.

## Non-Functional Requirements

- prefer explicit, low-magic module boundaries over clever abstractions
- keep dependency direction acyclic where practical
- optimize for correctness and diagnosability before reducing line count
- treat concurrency and failure paths as part of the primary design, not follow-up polish

## Edge Cases And Failure Cases

- Redis command connection loss during claim/resolve
- subscriber timeout / disconnect / reconnect
- stale cache view after node death or pub/sub lag
- `resolveRoom()` fallback when cache is cold
- claim takeover when node metadata is missing
- startup with Redis unavailable

## Open Questions

- whether the cache should remain embedded in `RoomRegistry` or move into an internal `RegistryCache` object
- whether command I/O should remain synchronous behind `cmdMutex_` or later move toward a more explicit command executor abstraction
