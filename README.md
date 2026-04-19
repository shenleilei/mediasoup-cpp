# mediasoup-cpp

A C++17 SFU control plane built around the upstream `mediasoup-worker`.

This project replaces mediasoup's usual Node.js control layer with a native C++ server while still relying on the battle-tested `mediasoup-worker` process for media handling. The result is a C++-native signaling stack with room/session management, multi-node room routing, recording, and QoS aggregation.

## Live Demo

Try it now: **http://47.99.237.234:3000**

Open in two browser tabs or two devices to start a 1-on-1 call with real-time QoS monitoring.

Recording playback: **http://47.99.237.234:3000/playback.html**

## Why

mediasoup is widely used, but its default control plane is Node.js. This project keeps the upstream C++ media worker and replaces the control plane with C++17 for:

- lower signaling overhead
- single-binary control-plane deployment
- tighter control over threading, failure handling, and integration with native systems

The media plane is still handled by `mediasoup-worker`, connected through Unix pipes and FlatBuffers IPC.

## What This Project Is

This project is:

- a WebSocket/HTTP signaling server
- a room / peer / session management layer
- a transport / producer / consumer orchestration layer
- a Redis-backed multi-node room routing layer
- a recorder / QoS integration layer

This project is not:

- a rewrite of mediasoup media internals
- a replacement for `mediasoup-worker`

## High-Level Architecture

```text
Browser
  │
  │ WebSocket / HTTP
  ▼
uWS Main Thread
  │
  ├─ SignalingServer main-thread glue
  ├─ SignalingServerWs request/session handling
  ├─ SignalingServerHttp route handling
  ├─ binds roomId -> WorkerThread
  └─ sends responses / notifications
  │
  ▼
WorkerThread Pool (N)
  │
  ├─ one serial event loop per WorkerThread
  ├─ owns a subset of mediasoup worker processes
  ├─ owns RoomManager + RoomService for assigned rooms
  ├─ drives Channel IPC via epoll
  └─ runs room business logic single-threaded
  │
  ▼
RoomService Facade
  │
  ├─ lifecycle slice
  ├─ media slice
  ├─ stats / QoS slice
  └─ downlink planning slice
  │
  ▼
Router / Transport / Producer / Consumer
  │
  ▼
Channel (FlatBuffers over Unix pipe)
  │
  ▼
mediasoup-worker process
  │
  ▼
RTP / SRTP / ICE / DTLS
```

## Linux PlainTransport Client

Besides the browser path, this repo also ships a Linux `PlainTransport C++ client` used for
end-to-end publish / QoS verification and matrix regression.

```text
MP4
  -> FFmpeg demux / decode
  -> per-track x264 re-encode (or H264 copy path)
  -> RTP packetize
  -> UDP
  -> mediasoup PlainTransport

WebSocket control plane
  -> join / plainPublish / clientStats / getStats
  -> qosPolicy / qosOverride notifications

RTCP side path
  -> SR / RR / RTT / jitter
  -> NACK / PLI / FIR
  -> per-track retransmission + keyframe cache
```

Current architecture docs:

- [docs/dependencies_cn.md](./docs/dependencies_cn.md)
- [docs/linux-client-architecture_cn.md](./docs/linux-client-architecture_cn.md)
- [docs/linux-client-multi-source-thread-model_cn.md](./docs/linux-client-multi-source-thread-model_cn.md)
- [docs/linux-client-threaded-implementation-checklist_cn.md](./docs/linux-client-threaded-implementation-checklist_cn.md)
- [docs/architecture_cn.md](./docs/architecture_cn.md)
- [docs/plain-client-qos-parity-checklist.md](./docs/plain-client-qos-parity-checklist.md)

## Multi-Node Routing Architecture

```text
SignalingServer
  │
  ├─ WorkerThread Pool
  │
  └─ Registry Worker Thread
       │
       └─ RoomRegistry
            ├─ Redis command connection
            ├─ local room cache
            ├─ local node cache
            ├─ room ownership claim / refresh / unregister
            └─ resolveRoom(roomId, clientIp)
                 │
                 └─ GeoRouter (optional)
                      ├─ ip2region lookup
                      ├─ country isolation
                      └─ ISP / distance based scoring
```

There is also a dedicated Redis subscriber thread that listens for node and room updates and refreshes local cache state.

## Features

- room-first design: when a peer produces, other peers in the room are auto-subscribed
- WorkerThread-based signaling: room control logic runs off the uWS main loop
- session identity: reconnect replaces the old connection and stale requests are rejected by `sessionId`
- Redis-backed room routing: room ownership, cache, pub/sub sync, degrade-to-local mode
- geo-aware routing: ip2region-based country / ISP / distance scoring
- H264 / VP8 recording: RTP depacketization and WebM muxing through FFmpeg
- QoS monitoring: server stats + client stats aggregation and periodic push
- worker crash recovery: child process respawn with rate limiting
- daemon mode: fork, PID file, structured logs

## QoS Status

This repo now includes:

- a full uplink QoS path across:
  - client-side publisher QoS state machine and ladder control
  - server-side `clientStats` ingestion, validation, aggregation, and automatic override generation
  - browser/node harnesses for publish / stale-seq / policy-update / automatic override / manual clear
  - browser loopback weak-network matrix execution and case-by-case reporting
- a downlink QoS path across:
  - subscriber-side `downlinkClientStats` ingestion, validation, storage, and controller execution
  - server-side hidden/pinned/size-based allocation, health-driven degrade/recovery, and priority handling
  - producer-side zero-demand `pauseUpstream` / `resumeUpstream` coordination for sustained all-hidden cases
  - browser harnesses for consumer control, downlink auto pause/resume, and priority competition under constrained downlink

Current downlink scope is subscriber receive control plus zero-demand publisher pause/resume coordination.
`dynacast` and room-level global bitrate budgeting remain follow-on work.

### Current checked status

Current repo docs and generated artifacts show:

- browser uplink matrix main gate: `43 / 43 PASS` (`2026-04-13`)
- PlainTransport C++ client matrix: `43 / 43 PASS` (`2026-04-16`)
- PlainTransport C++ client signaling harness: `PASS`
- C++ client QoS unit parity: `PASS`

Current scope note:

- uplink QoS main path is closed on both browser and PlainTransport C++ client
- downlink currently covers subscriber receive control plus zero-demand publisher pause/resume coordination
- `dynacast` and room-level global bitrate budgeting remain follow-on work

Source-of-truth links:

- QoS overall status: [docs/qos-status.md](./docs/qos-status.md)
- final summary: [docs/uplink-qos-final-report.md](./docs/uplink-qos-final-report.md)
- result summary: [docs/uplink-qos-test-results-summary.md](./docs/uplink-qos-test-results-summary.md)
- per-case final result: [docs/uplink-qos-case-results.md](./docs/uplink-qos-case-results.md)
- plain-client current status: [docs/plain-client-qos-status.md](./docs/plain-client-qos-status.md)
- plain-client parity checklist: [docs/plain-client-qos-parity-checklist.md](./docs/plain-client-qos-parity-checklist.md)
- plain-client matrix result: [docs/plain-client-qos-case-results.md](./docs/plain-client-qos-case-results.md)
- downlink current status: [docs/downlink-qos-status.md](./docs/downlink-qos-status.md)
- linux client architecture: [docs/linux-client-architecture_cn.md](./docs/linux-client-architecture_cn.md)
- test coverage map: [docs/qos-test-coverage_cn.md](./docs/qos-test-coverage_cn.md)
- generated matrix artifact: [docs/generated/uplink-qos-matrix-report.json](./docs/generated/uplink-qos-matrix-report.json)

## Core Runtime Model

### uWS Main Thread

The main thread is responsible for:

- WebSocket accept / close / message handling
- HTTP endpoints
- room-to-thread dispatch
- socket/session ownership
- deferred sends back to the client event loop

Important invariant:

- **same room -> same WorkerThread**

The first successful join binds a `roomId` to a specific `WorkerThread` in the main thread before the business task is executed.

### WorkerThread

Each `WorkerThread` is a serial event loop that owns:

- zero or more mediasoup worker child processes
- one `RoomManager`
- one `RoomService`
- one task queue
- epoll registrations for worker pipe fds
- health / GC timers

Inside a `WorkerThread`, room logic is intentionally single-threaded. That keeps room state coherent without pervasive fine-grained locking.

### mediasoup Worker Processes

Each `WorkerThread` owns a subset of mediasoup worker child processes.

Those child processes:

- create routers
- create transports
- negotiate media
- forward RTP
- expose stats over IPC

### Registry Worker

Redis fire-and-forget maintenance tasks are not executed inline on room control paths. A dedicated registry worker thread handles:

- room TTL refresh
- room unregister
- node heartbeat / load update

This keeps steady-state room control paths less sensitive to Redis latency.

## Thread Model

| Thread | Count | Role |
|---|---:|---|
| uWS main | 1 | WebSocket, HTTP, timers, room dispatch |
| WorkerThread | N | serial room logic + epoll-driven worker IPC |
| Registry worker | 1 | async Redis maintenance tasks |
| Redis subscriber | 1 | pub/sub cache updates |
| Worker waiter | per worker | child process wait / death handling |
| Recorder | per active recorder | UDP RTP receive + muxing |

Typical small deployment:

- 1 uWS main thread
- 1 registry worker
- 1 Redis subscriber
- 1 WorkerThread
- 1 mediasoup worker process
- 0..M recorder threads

## Startup Sequence

All critical modules are initialized before the server begins accepting traffic:

1. `RoomRegistry`: Redis connect, `syncAll()`, subscriber thread
2. `WorkerThread::start()`: create worker processes
3. `waitReady()`: block until WorkerThreads report initialization complete
4. `uWS::App().listen()`: only then begin accepting WebSocket / HTTP connections

## Session Identity Model

The project distinguishes:

- business identity: `peerId`
- connection identity: `sessionId`

This matters for reconnect handling:

- a new join for the same `peerId` replaces the old session
- the old socket is invalidated
- the `Peer` object is stamped with the new `sessionId`
- stale requests from the replaced connection are rejected before mutating room state

## Data Flow

### Join

```text
client -> WebSocket join
      -> uWS main thread
      -> assign roomId -> WorkerThread
      -> WorkerThread executes RoomService::join()
      -> RoomManager creates room if needed
      -> response deferred back to main loop
      -> socket gets roomId / peerId / sessionId bound
```

### Produce

```text
client -> produce
      -> uWS main thread
      -> dispatch to room's WorkerThread
      -> session validation
      -> RoomService::produce()
      -> Transport::produce()
      -> Channel::requestWait()
      -> mediasoup-worker creates Producer
      -> auto-subscribe other peers
      -> optionally start recorder path
      -> response deferred back to client
```

### Stats / QoS

```text
timer -> main thread
      -> posts stats work to each WorkerThread
      -> WorkerThread walks rooms / peers
      -> gathers transport / producer / consumer stats
      -> merges clientStats
      -> broadcasts statsReport
      -> recorder may append QoS snapshots
```

### PlainTransport Linux Client

```text
linux plain-client
      -> WebSocket join / plainPublish
      -> UDP RTP to PlainTransport
      -> RTCP SR / RR / NACK / PLI
      -> async getStats-assisted sampling
      -> PublisherQosController per video track
      -> clientStats snapshots
      -> qosPolicy / qosOverride notifications
```

### Media

The media plane does not pass through the signaling logic after setup:

```text
Browser A ──SRTP/UDP──→ WebRtcTransport → Producer
                                            ├──→ Consumer (SIMPLE) → WebRtcTransport → Browser B
                                            └──→ Consumer (PIPE) → PlainTransport
                                                    │ localhost UDP RTP
                                                    ▼
                                              PeerRecorder → .webm
```

## Room / Peer Model

### Room

A room owns:

- a `Router`
- a peer map
- room activity timestamps

### Peer

A peer owns:

- `peerId`
- `displayName`
- `sessionId`
- RTP capabilities
- send transport
- recv transport
- producers
- consumers

### Auto-Subscribe

The room model is room-first rather than explicit per-subscription signaling.

When one peer produces:

- all other peers with a recv transport are auto-subscribed
- they receive `newConsumer` notifications

## Redis / Room Ownership

`RoomRegistry` is responsible for:

- node registration
- node load publication
- room ownership claim
- room ownership refresh
- room lookup / resolution
- room and node cache maintenance

Important behavior:

- room ownership is stored in Redis
- room and node info are cached locally
- pub/sub keeps cache warm
- `resolveRoom()` can fall back to a Redis-backed node refresh if local cache is missing fresh node data
- when Redis is unavailable, the system degrades to local-only mode

## Geo Routing

If `GeoRouter` is enabled:

- client IP is mapped via `ip2region`
- same-country routing can be enforced
- candidate nodes are ranked by:
  - country isolation
  - ISP affinity
  - geographic distance
  - current load

This logic lives under `RoomRegistry`, not directly under `RoomService`.

### Country Isolation

Enabled by default.

- Chinese IP -> Chinese nodes only
- US IP -> US nodes only

Disable with `--noCountryIsolation` or `"countryIsolation": false`.

### Example

```bash
# Hangzhou node (China Telecom)
./build/mediasoup-sfu \
  --nodaemon \
  --port=3000 \
  --announcedIp=<public-ip> \
  --lat=30.27 \
  --lng=120.15 \
  --isp=电信 \
  --country=中国

# US West node
./build/mediasoup-sfu \
  --nodaemon \
  --port=3001 \
  --announcedIp=<public-ip> \
  --lat=37.39 \
  --lng=-122.08 \
  --isp=Amazon \
  --country="United States"
```

## Recording

Recording is implemented as a side path:

```text
Producer
  -> PIPE Consumer
  -> PlainTransport
  -> localhost UDP RTP
  -> PeerRecorder
  -> FFmpeg / WebM output
```

Recorder responsibilities include:

- RTP receive
- H264 / VP8 packet handling
- muxing
- QoS timeline sidecar output

## Quick Start

### Prerequisites

Dependency reference:

- [docs/dependencies_cn.md](./docs/dependencies_cn.md)

- Linux
- CMake 3.16+
- GCC 10+ or Clang 12+
- OpenSSL
- zlib
- FFmpeg
  - `libavformat`
  - `libavcodec`
  - `libavutil`
  - `libswscale`
  - `libavdevice`
- hiredis
- `curl` and `tar` (used by `setup.sh` to fetch and unpack `mediasoup-worker`)

Notes:

- Redis is optional at runtime for single-node local-only mode, but the current default build still links `hiredis`.
- The current `plain-client` threaded path directly includes `libavdevice`, so `libavdevice-dev` / `ffmpeg-devel` is part of the build requirement, not just a camera-only afterthought.

### Build

```bash
git clone --recursive https://github.com/user/mediasoup-cpp.git
cd mediasoup-cpp
./setup.sh
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### Run

Always run from the project root.

Minimal:

```bash
./build/mediasoup-sfu --nodaemon --port=3000 --listenIp=0.0.0.0
```

Recommended production-style invocation:

```bash
./build/mediasoup-sfu \
  --nodaemon \
  --port=3000 \
  --workers=1 \
  --workerThreads=1 \
  --listenIp=0.0.0.0 \
  --announcedIp=<public-ip> \
  --workerBin=./mediasoup-worker \
  --redisHost=127.0.0.1 \
  --redisPort=6379 \
  --nodeId=<unique-node-id>
```

With config file:

```bash
cat > config.json <<'EOF'
{
  "port": 3000,
  "workers": 1,
  "workerThreads": 1,
  "workerBin": "./mediasoup-worker",
  "listenIp": "0.0.0.0",
  "announcedIp": "<public-ip>",
  "redisHost": "127.0.0.1",
  "redisPort": 6379,
  "recordDir": "./recordings",
  "logDir": "/var/log/mediasoup",
  "logPrefix": "mediasoup-sfu",
  "logRotateHours": 3
}
EOF

./build/mediasoup-sfu --nodaemon --config=config.json
```

Open `http://<server-ip>:3000`.

## Command-Line Options

| Option | Default | Description |
|---|---|---|
| `--port` | `3000` | signaling + HTTP port |
| `--workers` | CPU based | mediasoup worker process count |
| `--workerThreads` | auto | WorkerThread event loop count |
| `--listenIp` | `0.0.0.0` | transport listen IP |
| `--announcedIp` | auto-detect | public IP for ICE candidates |
| `--workerBin` | `./mediasoup-worker` | worker binary path |
| `--recordDir` | `./recordings` | recording output directory |
| `--logDir` | `/var/log/mediasoup` | daemon log directory |
| `--logPrefix` | `mediasoup-sfu` | daemon log file prefix |
| `--logLevel` | `info` | log verbosity |
| `--logRotateHours` | `3` | rotate daemon log every N hours into files like `mediasoup-sfu_2026041306_<pid>.log` (`0` disables rotation) |
| `--nodaemon` | flag | run in foreground |
| `--redisHost` | `127.0.0.1` | Redis host |
| `--redisPort` | `6379` | Redis port |
| `--nodeId` | auto | node identifier |
| `--nodeAddress` | auto | externally advertised WS address |
| `--lat` | auto-detect | node latitude |
| `--lng` | auto-detect | node longitude |
| `--isp` | auto-detect | node ISP |
| `--country` | auto-detect | node country |
| `--countryIsolation` | on | same-country routing only |
| `--noCountryIsolation` | flag | disable country isolation |
| `--geoDb` | `./ip2region.xdb` | ip2region database path (falls back to `./third_party/ip2region/ip2region.xdb`) |

If `./ip2region.xdb` is not present, the server also checks the vendored source copy at `./third_party/ip2region/ip2region.xdb` and the executable's build directory copy.

## Important Deployment Notes

### 1. Explicitly set `announcedIp` in production

The code has best-effort auto detection, but production deployment should not rely on it.

Set:

- `--announcedIp`
- optionally `--nodeId`
- optionally `--nodeAddress` if your environment has special routing or proxy topology

### 2. Run tests from repo root

The integration tests spawn binaries using relative paths under `./build`.

### 3. Multi-node requires reachable node addresses

If Redis multi-node routing is enabled, every node must publish a `ws://` address reachable by clients or by the upstream proxy layer.

## Testing

All tests must be run from the project root directory.

```bash
# full repository regression
./scripts/run_all_tests.sh

# QoS JS / harness / matrix regression
./scripts/run_qos_tests.sh

# individual binaries
./build/mediasoup_tests
./build/mediasoup_qos_unit_tests
./build/mediasoup_review_fix_tests
./build/mediasoup_stability_integration_tests
./build/mediasoup_multinode_tests
./build/mediasoup_topology_tests
./build/mediasoup_integration_tests
./build/mediasoup_qos_integration_tests
./build/mediasoup_e2e_tests
./build/mediasoup_bench
```

`mediasoup_review_fix_tests`, `mediasoup_multinode_tests`, and `mediasoup_topology_tests`
start an isolated `redis-server` automatically. They require the `redis-server` binary in
`PATH`, but they do not rely on a shared Redis on `127.0.0.1:6379`.

`./scripts/run_all_tests.sh` and `./scripts/run_qos_tests.sh` both keep running the remaining
selected test groups after a test failure and return non-zero only after printing a final
failure summary.

`./scripts/run_all_tests.sh` also rewrites the latest full regression report:
[docs/full-regression-test-results.md](./docs/full-regression-test-results.md)

For unattended nightly execution, use the repo-local wrapper:

```bash
cp .nightly-full-regression.env.example .nightly-full-regression.env
./scripts/nightly_full_regression.py run
./scripts/install_nightly_full_regression_cron.sh
```

The nightly wrapper stores a timestamped run directory under
`artifacts/nightly-full-regression/`, refreshes `/var/log/run_all_tests.log` by default,
and emails the pass-rate / failed-case summary with selected Markdown report attachments.
See [docs/nightly-full-regression.md](./docs/nightly-full-regression.md).

When the run includes `qos`, it delegates that slice to `./scripts/run_qos_tests.sh`, so the
QoS-specific summaries and matrix artifacts owned by that script are refreshed as part of the run.

The regression-heavy suites currently cover:

- reconnect semantics
- stale request rejection
- restartIce correctness
- non-blocking stats path
- recording path stability
- geo routing
- country isolation
- Redis degrade mode
- cache propagation
- full-node redirect behavior

### Quick Quality Gate

For local verification before review:

```bash
# configure + build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# full repository regression
./scripts/run_all_tests.sh

# fast baseline (core unit)
./build/mediasoup_tests

# QoS unit baseline
./build/mediasoup_qos_unit_tests
```

### QoS Test Entry

For QoS-specific validation, use the unified script:

```bash
cd /root/mediasoup-cpp

# full QoS run
./scripts/run_qos_tests.sh

# continue from last failed tasks only
./scripts/run_qos_tests.sh --resume

# skip browser-dependent parts
./scripts/run_qos_tests.sh --skip-browser

# run selected groups
./scripts/run_qos_tests.sh client-js cpp-unit

# default browser matrix gate
node tests/qos_harness/run_matrix.mjs

# extended browser matrix (adds the remaining extended baseline cases)
node tests/qos_harness/run_matrix.mjs --include-extended

# targeted blind-spot rerun
node tests/qos_harness/run_matrix.mjs --cases=T9,T10,T11

# multi-room capacity ramp:
# each room has exactly 2 peers: 1 publisher sending 1080p, 1 subscriber receiving it
node tests/qos_harness/browser_capacity_rooms.mjs --workers=1 --step=5 --max-rooms=50
```

Behavior:

- default mode runs all QoS groups and continues even if one group fails
- failures are recorded to `tests/qos_harness/artifacts/last-failures.txt`
- `--resume` reruns only the last failed precise tasks
- if `matrix` is executed, the script also regenerates the per-case report:
  [docs/uplink-qos-case-results.md](./docs/uplink-qos-case-results.md)
- the default matrix now includes the blind-spot transition cases `T9/T10/T11`; the remaining `extended` set is currently the higher-bandwidth baseline calibration cases and can be added with `--include-extended`, or targeted explicitly via `--cases=...`

### Troubleshooting

- `Cannot find source file ... third_party/ip2region/binding/c/xdb_searcher.c`  
  Ensure you are on the latest branch and the bundled `third_party/ip2region` directory exists.
- `Could NOT find ... avformat/avcodec/avutil`  
  Install FFmpeg development packages (for example `libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libavdevice-dev` on Debian/Ubuntu), or see [docs/dependencies_cn.md](./docs/dependencies_cn.md).
- `hiredis not found` or link errors for Redis symbols  
  Install the hiredis development package (`libhiredis-dev` / `hiredis-devel`), or see [docs/dependencies_cn.md](./docs/dependencies_cn.md).

## Monitoring

Production monitoring lives under `deploy/monitoring`.

### Quick Start

```bash
cd deploy/monitoring
docker compose up -d
```

### Dashboards

| Service | URL |
|---|---|
| Grafana | `http://<server-ip>:3001` |
| Prometheus | `http://<server-ip>:9090` |
| Alertmanager | `http://<server-ip>:9093` |

Live demo Grafana: **http://47.99.237.234:3001**

## Performance

Tested on Intel Xeon Platinum 2.5GHz, 2 vCPU:

| Metric | Loopback | Real Network |
|---|---:|---:|
| Peak rooms (1P + 2C each) | 240 | 80 |
| Worker CPU | 82% | 23% |
| RSS | 180 MB | 67 MB |
| PPS (in -> out) | 72k -> 144k | 24k -> 48k |

Real-world estimate: roughly **30-40** 1v1 rooms per mediasoup worker for typical audio + video WebRTC traffic.

## Project Structure

```text
src/
├── main.cpp              # thin process entry + signal wiring
├── MainBootstrap.*       # runtime options, geo/bootstrap, worker-thread pool creation
├── RuntimeDaemon.*       # daemonize + startup notification plumbing
├── Constants.h           # runtime constants
├── SignalingServer.h     # signaling server facade
├── SignalingServerWs.*   # WebSocket request/session dispatch
├── SignalingServerHttp.* # HTTP routes, metrics, file serving
├── SignalingServerRuntime.cpp # runtime snapshot, registry worker, room dispatch helpers
├── SignalingSocketState.h     # ws session / rate-limit helpers
├── SignalingRequestDispatcher.h # method -> RoomService dispatch glue
├── StaticFileResponder.h      # static-file path resolution + streaming
├── WorkerThread.*        # epoll event loop per signaling worker thread
├── RoomService.h         # room-service facade
├── RoomServiceLifecycle.cpp # join/leave/health/cleanup
├── RoomServiceMedia.cpp  # transport / produce / consume flows
├── RoomServiceStats.cpp  # stats / QoS / room-state broadcast
├── RoomServiceDownlink.cpp # downlink planning + publisher supply
├── RoomRecordingHelpers.* # recorder setup / cleanup helpers
├── RoomMediaHelpers.h    # media-side helper routines
├── RoomDownlinkHelpers.h # downlink helper routines
├── RoomStatsQosHelpers.h # stats/QoS helper routines
├── RoomManager.h         # room container and lifecycle
├── RoomRegistry.*        # Redis routing, cache, pub/sub sync
├── GeoRouter.h           # geolocation and scoring
├── WorkerManager.h       # worker selection / capacity helpers
├── Worker.*              # mediasoup-worker child process wrapper
├── Channel.*             # FlatBuffers IPC over Unix pipes
├── Router.*              # router wrapper
├── Transport.*           # transport wrapper
├── WebRtcTransport.*     # ICE / DTLS transport
├── PlainTransport.h      # plain RTP transport for recorder path
├── Producer.*            # producer wrapper
├── Consumer.*            # consumer wrapper
├── Peer.h                # peer + session state
├── Recorder.h            # RTP -> WebM recording and QoS timeline
├── EventEmitter.h        # lightweight event system
└── Logger.h              # spdlog wrapper
```

## License

MIT — see [LICENSE](LICENSE).

## Acknowledgments

- [mediasoup](https://mediasoup.org/)
- [uWebSockets](https://github.com/uNetworking/uWebSockets)
- [FlatBuffers](https://google.github.io/flatbuffers/)
- [nlohmann/json](https://github.com/nlohmann/json)
- [spdlog](https://github.com/gabime/spdlog)
- [FFmpeg](https://ffmpeg.org/)
- [ip2region](https://github.com/lionsoul2014/ip2region)
