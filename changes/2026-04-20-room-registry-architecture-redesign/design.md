# Design

## Design Standard

This is not a cosmetic file split.

The goal is to make `RoomRegistry` behave like a deliberately designed subsystem with:

- a stable facade
- explicit internal modules
- explicit state ownership
- explicit lock and thread rules
- explicit verification requirements

## Stable Facade

The public `RoomRegistry` API should remain the main entrypoint for callers:

- `start()`
- `stop()`
- `heartbeat()`
- `updateLoad()`
- `refreshRooms()`
- `claimRoom()`
- `resolveRoom()`
- `refreshRoom()`
- `unregisterRoom()`

Callers should not need to know the internal module graph.

## Internal Modules

The highest-standard target architecture should separate `RoomRegistry` into these internal roles.

### 1. Command Connection

Responsibilities:

- own the main Redis command `redisContext*`
- reconnect on demand
- expose the minimal synchronous command primitives required by higher layers

Owned state:

- `ctx_`
- `cmdMutex_`
- Redis host/port config

Forbidden responsibilities:

- cache mutation
- selection policy
- pub/sub

### 2. Cache View

Responsibilities:

- own in-memory `nodeCache_` and `roomCache_`
- provide safe read/update operations
- centralize cache mutation paths

Owned state:

- `nodeCache_`
- `roomCache_`
- `cacheMutex_`

Forbidden responsibilities:

- Redis I/O
- reconnect logic

### 3. Sync Engine

Responsibilities:

- bulk sync nodes/rooms from Redis into a fresh temporary view
- apply synchronized results to cache in one mutation step
- perform dead-node eviction checks

Dependencies:

- command connection
- cache view
- reply parsing helpers

### 4. PubSub Worker

Responsibilities:

- own subscriber-thread lifecycle
- own subscriber connection lifecycle
- parse pub/sub messages and translate them into cache updates

Owned state:

- `subThread_`
- `subStop_`
- subscriber `redisContext*` (thread-local to subscriber loop, not shared)

Forbidden responsibilities:

- claim/resolve business decisions
- use of command connection `ctx_`

### 5. Selection And Claim Policy

Responsibilities:

- room claim decision flow
- room resolve decision flow
- best-node selection from cache

Dependencies:

- cache view
- sync engine (only through explicit refresh points)
- `GeoRouter`
- Redis command connection for claim/lookup operations that must touch Redis

## Dependency Graph

Target dependency direction:

```text
RoomRegistry facade
  -> CommandConnection
  -> CacheView
  -> SyncEngine
  -> PubSubWorker
  -> SelectionAndClaimPolicy

SyncEngine
  -> CommandConnection
  -> CacheView
  -> ReplyUtils

PubSubWorker
  -> CacheView
  -> ReplyUtils

SelectionAndClaimPolicy
  -> CacheView
  -> CommandConnection
  -> SyncEngine
  -> GeoRouter
  -> RoomRegistrySelection helpers
```

The important point is:

- pub/sub should not depend on claim/resolve policy
- cache should not depend on Redis command execution
- command connection should not know cache semantics

## Ownership Rules

### Redis command context

- exactly one owner: command connection
- all access serialized by `cmdMutex_`

### Subscriber context

- exactly one owner: subscriber thread
- never shared with command connection

### Cache maps

- exactly one owner: cache view
- all mutation through cache-owned methods or clearly bounded cache update sites

### Geo router pointer

- observed by selection policy
- not owned by `RoomRegistry`

## Lock And Thread Rules

### Rule 1

No Redis I/O while holding `cacheMutex_`.

### Rule 2

`cmdMutex_` protects only command connection state and direct Redis command execution.

### Rule 3

The subscriber thread may mutate cache, but only through cache-owned mutation points guarded by `cacheMutex_`.

### Rule 4

No code path should hold both `cmdMutex_` and `cacheMutex_` across an external blocking operation.

### Rule 5

Bulk sync operations should gather remote data first, then acquire `cacheMutex_` once to publish the new view.

## Data Flow

### Claim Flow

```text
claimRoom()
  -> cache fast-path
  -> command connection Lua/EVAL path
  -> selection fallback if needed
  -> cache publication
  -> optional pub/sub publication
```

### Resolve Flow

```text
resolveRoom()
  -> cache fast-path
  -> command connection GET room / GET owner node
  -> sync-nodes fallback if cache cold
  -> selection policy on cache
  -> cache publication if owner resolved
```

### Subscriber Flow

```text
PubSubWorker thread
  -> subscribe
  -> poll
  -> parse message
  -> cache mutation
  -> reconnect + syncAll() on disconnect
```

## Implementation Phases

### Phase 1: Codify current architecture with explicit files

Use the current method split as a transitional step:

- `RoomRegistryConnection.cpp`
- `RoomRegistryPubSub.cpp`
- `RoomRegistrySync.cpp`
- `RoomRegistry.cpp`

but document them as temporary file boundaries, not the final architecture.

### Phase 2: Introduce explicit internal helper types

Create internal classes or structs for:

- command connection
- cache view
- pub/sub worker
- sync engine

without changing facade callers.

### Phase 3: Route facade methods through the new internal owners

Move method logic from free-floating `RoomRegistry::*` method clusters to explicit owner objects.

### Phase 4: Tighten tests and operational diagnostics

Add or update verification to cover:

- reconnect paths
- cache cold/warm paths
- dead-node eviction
- subscriber disconnect and resync

## Verification Matrix

### Build

- `mediasoup-sfu`
- multinode/integration targets touching `RoomRegistry`

### Unit / focused checks

- selection ordering
- reply parsing validation
- cache update behavior where practical

### Integration

- `mediasoup_integration_tests`
- `mediasoup_multinode_tests`
- `mediasoup_topology_tests`

### Operational regression

- subscriber CPU does not regress to busy-loop behavior
- Redis unavailable startup still degrades cleanly

## Risks And Mitigations

- Risk: internal helper types add indirection without real ownership clarity.
  Mitigation: each helper must own distinct state, not just wrap a method pile.

- Risk: cache and command code still leak into each other through convenience calls.
  Mitigation: treat dependency direction as a hard design rule.

- Risk: pub/sub reconnect or sync semantics regress during migration.
  Mitigation: keep phase ordering incremental and verify each phase with multinode tests.

## Recommended Next Implementation Order

1. Stop doing opportunistic method-by-method moves.
2. Introduce an explicit cache owner.
3. Introduce an explicit command connection owner.
4. Move pub/sub thread lifecycle behind a dedicated internal worker object.
5. Only then refactor claim/resolve around those stable owners.
