# Development Notes — 2026-04-08

## Changes Summary

Two major areas of work: code review fixes and geo-aware routing.

## 1. Code Review Fixes

Based on `docs/CODE_REVIEW_2026-04-08.md`, 5 original findings + issues discovered during review iterations.

### Original Findings Fixed

| # | Severity | Issue | Fix |
|---|----------|-------|-----|
| 1 | High | Duplicate peerId reconnect overwrites session, old close kicks new | `replacePeer()` + router producer cleanup + recorder/clientStats cleanup + kick old WebSocket + `peerJoined` with `reconnect:true` |
| 2 | High | Redis room takeover not atomic (split-brain risk) | Lua script for atomic claim; node value returned directly from Lua (no second GET) |
| 3 | High | `restartIce()` returns stale ICE credentials | Parse `RestartIceResponse`, update `iceParameters_` with null safety |
| 4 | Medium | `collectPeerStats()` pseudo-timeout blocks control thread | Remove `std::async`, use `getStats(timeoutMs)` with 500ms per-call + 2s per-peer budget |
| 5 | Medium | `PlainTransport::connect()` no timeout | Use `requestWait()` with 5s default timeout |

### Issues Found During Review Iterations

- `restartIce` null pointer risk → added 3-level null check
- `Room::addPeer` close-under-lock → moved close outside mutex
- `collectPeerStats` missing `catch(...)` → restored
- Lua KEYS parameter misuse → numkeys=1, prefix in ARGV
- Old WebSocket still sends control requests after reconnect → kick via `ws->end()`
- `addPeer` close bypasses router cleanup → `replacePeer()` returns old peer for caller to clean
- Reconnect doesn't clean recorder/clientStats → replicated `leave()` cleanup path
- `peerReconnected` breaks wire protocol → changed to `peerJoined` + `reconnect:true`
- `resolveRoom`/`claimRoom` disagree on owner lookup failure → Lua returns node value directly
- `claimRoom` throws on transient Redis failure → graceful degradation

### Files Modified

`RoomManager.h`, `RoomService.cpp`, `RoomService.h`, `SignalingServer.cpp`, `RoomRegistry.h`, `WebRtcTransport.cpp`, `PlainTransport.h`, `Transport.h/cpp`, `Producer.h/cpp`, `Consumer.h/cpp`, `Constants.h`, `CMakeLists.txt`

### Tests Added

- `test_review_fixes.cpp` — 8 unit tests (duplicate peer, stats timeout, ICE params)
- `test_review_fixes_integration.cpp` — 6 integration tests (reconnect, restartIce, stats, recording)

## 2. Geo-Aware Node Routing

### Design

Client IP → ip2region lookup → geographic coordinates + ISP → score candidate nodes → select best.

### Scoring Formula

**China (domestic):**
- Same ISP or cloud ISP: `score = distance_km`
- Cross-ISP (traditional carriers): `score = max(distance_km, 50) × 2.0`
- 2.0x multiplier based on real-world data: cross-ISP BGP routing can detour through distant exchange points, causing 2-4x latency

**Overseas:** `score = distance_km` (no ISP penalty)

**Tiebreaker:** scores within 100km → sort by room count (load)

### Cloud ISP Detection

Multi-line BGP providers are ISP-neutral (no cross-ISP penalty): 阿里, 腾讯, 华为, 百度, 京东, 字节, 网宿, 白山, 七牛, 又拍, 金山, 天翼云, 移动云, 联通云, 鹏博士

### Country Isolation

Default: enabled. Clients only routed to same-country nodes. Disable with `--noCountryIsolation`.

### City Coordinates

339 entries: 293 Chinese prefecture-level cities + 27 provinces (fallback) + 3 HK/Macau/Taiwan + ~60 overseas (English keys matching ip2region output).

### Both Entry Paths Support Geo

- `/api/resolve?roomId=xxx` — extracts client IP from query param / X-Forwarded-For / remote address
- Direct WebSocket `join` — extracts client IP from WS connection; `claimRoom` checks `findBestNode`, redirects if better node exists

### Capacity Handling

- All nodes full → returns `"no available nodes"` error
- Local node full (degraded mode) → returns `"no available capacity"` error

### Files Added/Modified

- `GeoRouter.h` — new: ip2region wrapper, city coords, ISP scoring
- `RoomRegistry.h` — extended: geo params, `findBestNodeCached()`, country isolation
- `main.cpp` — new CLI params: `--lat`, `--lng`, `--isp`, `--country`, `--countryIsolation`, `--geoDb`
- `SignalingServer.cpp` — client IP extraction for resolve and join
- `RoomService.h/cpp` — `clientIp` parameter threading
- `CMakeLists.txt` — ip2region library build
- `ip2region.xdb` — database file (11MB)

### Tests Added

- `test_geo_router.cpp` — 17 unit tests (lookup, haversine, ISP classification, scoring, ranking)
- Integration: 2 geo routing tests, 4 country isolation tests

## 3. Redis Local Cache + Pub/Sub

### Problem

Every `resolveRoom` and `findBestNode` call hit Redis (KEYS + GET×N). High latency, single point of failure.

### Solution

Local in-memory cache + Redis pub/sub for real-time sync.

### Cache Structure

```cpp
unordered_map<string, NodeInfo> nodeCache_;   // nodeId → {address, rooms, maxRooms, lat, lng, isp, country}
unordered_map<string, string> roomCache_;     // roomId → ownerAddress
```

### Pub/Sub Channels

- `sfu:nodes` — message format: `nodeId=nodeValue`
- `sfu:rooms` — message format: `roomId=ownerAddress` (empty = removed)

### Query Path (Zero Redis)

| Operation | Before | After |
|-----------|--------|-------|
| `resolveRoom` | GET + KEYS + GET×N | Local cache lookup |
| `findBestNode` | KEYS + GET×N | Iterate `nodeCache_` |
| `claimRoom` (cached) | EVAL | Local cache check, skip EVAL |

### Write Path (Still Redis)

| Operation | Redis calls | Then |
|-----------|-------------|------|
| `claimRoom` (new) | EVAL | Update cache + PUBLISH |
| `updateLoad` | SET | Update cache + PUBLISH |
| `unregisterRoom` | DEL | Update cache + PUBLISH |

### Sync Mechanism

1. **Startup**: full sync (KEYS + GET all nodes and rooms)
2. **Runtime**: dedicated subscriber thread, SUBSCRIBE to both channels
3. **Reconnect**: re-subscribe + full sync to catch missed events
4. **Redis down**: local cache continues serving until data becomes stale

### Subscriber Thread

- Dedicated `redisContext` (separate from command connection)
- 2-second read timeout for responsive shutdown
- Auto-reconnect with 2-second retry interval

### Tests Added

- 3 integration tests: room propagation via pub/sub, cache hit performance, cross-node redirect via cache
- 2 Redis degradation tests: resolve and join work when Redis is unreachable

## 4. Redis Graceful Degradation

### Behavior Matrix

| Redis State | New Room | Existing Room | Recovery |
|-------------|----------|---------------|----------|
| Normal | Multi-node geo routing | Normal | — |
| Disconnected | Local creation (no routing) | Unaffected (in-memory) | Heartbeat auto-reconnects |
| EVAL failed | Local creation | Unaffected | Next operation retries |

### Local Capacity Check

Before creating a room locally, `join()` checks `roomCount >= maxTotalRouters`. Returns `"no available capacity"` if full.

## Test Summary

| Suite | Count | Coverage |
|-------|-------|----------|
| Unit tests | 143 | ORTC, room/peer, stability, worker queue, request timeout, QoS, geo router |
| Integration (original) | 17 | Join/leave, transport, produce, auto-subscribe, room isolation, recording |
| Review fix integration | 17 | Reconnect, restartIce, stats, recording, geo routing, country isolation, Redis degradation, cache/pub-sub |
| **Total** | **160+** | |
