# mediasoup-cpp

English | [简体中文](./README.md)

A C++17 SFU control plane built around the upstream `mediasoup-worker`.

This project replaces mediasoup's usual Node.js control layer with a native C++ server while still relying on the battle-tested `mediasoup-worker` process for media handling. The result is a C++-native signaling stack with room/session management, local room routing, and QoS aggregation.

## Live Demo

Try it now: **http://47.99.237.234:3000**

Open in two browser tabs or two devices to start a 1-on-1 call with real-time QoS monitoring.

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
- a local room-routing layer
- a QoS aggregation layer

This project is not:

- a rewrite of mediasoup media internals
- a replacement for `mediasoup-worker`

## Core Value

This project does not rewrite the mediasoup media worker. It keeps the upstream worker and builds the surrounding C++ control plane, browser demo, QoS aggregation, and operations surface into something deployable, testable, and observable.

### 1. C++ Control Plane And Room Threading
- **C++17 control plane:** `uWebSockets` handles HTTP/WebSocket while media remains in the upstream `mediasoup-worker`.
- **Room-serial execution:** each room is bound to a `WorkerThread`, reducing cross-thread room-state sharing and lock contention.
- **FlatBuffers IPC:** the control plane talks to `mediasoup-worker` through Unix pipes and FlatBuffers with explicit protocol boundaries.

### 2. Browser Interop And Server PlainTransport
- **Browser-first path:** the browser demo keeps WebSocket signaling, room join, publish, subscribe, and realtime QoS display.
- **Server capability retained:** `plainPublish` / `plainSubscribe` remain server PlainTransport protocol capabilities for tests and future server-side media ingress.
- **Native client removed:** the root `client/` WebRTC QoS plain push/play client has been removed, and the default build no longer depends on the external WebRTC QoS SDK package.

### 3. Reproducible QoS Validation
- **Browser uplink matrix:** browser/Node harnesses validate the uplink QoS state machine and server aggregation path.
- **Server QoS regression:** C++ QoS unit tests, integration tests, browser harnesses, and uplink/downlink matrices remain active.
- **Explicit environment SKIP rules:** missing browser or netem prerequisites must be recorded as `SKIP/PARTIAL`, not counted as PASS.

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

## Simplified Service Boundary

The repo keeps `mediasoup-sfu`, the browser demo, WebSocket signaling, room interop, and server-side QoS/PlainTransport capabilities. The root `client/` native WebRTC QoS plain push/play client, its scripts, harnesses, unit tests, and generated reports have been removed.

The default build needs only server and test dependencies. It no longer searches for or requires the external WebRTC QoS SDK package:

```bash
cmake -S . -B build-slim -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake --build build-slim --target mediasoup-sfu -j$(nproc)
```

## Local Runtime Architecture

```text
SignalingServer
  │
  ├─ WorkerThread Pool
  └─ GeoRouter (optional)
       ├─ ip2region lookup
       ├─ country isolation
       └─ ISP / distance based scoring
```

## Features

- room-first design: when a peer produces, other peers in the room are auto-subscribed
- WorkerThread-based signaling: room control logic runs off the uWS main loop
- session identity: reconnect replaces the old connection and stale requests are rejected by `sessionId`
- local single-node room routing
- geo-aware routing: ip2region-based country / ISP / distance scoring
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

- browser uplink matrix: original `43 / 43 PASS` main gate (`2026-04-13`) plus `GD1-GD12` targeted PASS; current total scope is `55 case`
- server QoS unit tests, integration tests, and browser harnesses remain the current regression surface.
- the root native WebRTC QoS plain push/play client has been removed; its P2/P3 reports and acceptance scripts are no longer active paths.

Current scope note:

- uplink QoS is currently validated through browser and Node/browser harnesses
- downlink currently covers subscriber receive control plus zero-demand publisher pause/resume coordination
- server PlainTransport protocol capabilities remain, but this repo no longer ships native plain push/play clients
- `dynacast` and room-level global bitrate budgeting remain follow-on work

Source-of-truth links:

- QoS overall status: [docs/qos-status.md](./docs/qos-status.md)
- final summary: [docs/uplink-qos-final-report.md](./docs/uplink-qos-final-report.md)
- result summary: [docs/uplink-qos-test-results-summary.md](./docs/uplink-qos-test-results-summary.md)
- per-case final result: [docs/uplink-qos-case-results.md](./docs/uplink-qos-case-results.md)
- downlink current status: [docs/downlink-qos-status.md](./docs/downlink-qos-status.md)
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

## Thread Model

| Thread | Count | Role |
|---|---:|---|
| uWS main | 1 | WebSocket, HTTP, timers, room dispatch |
| WorkerThread | N | serial room logic + epoll-driven worker IPC |
| Worker waiter | per worker | child process wait / death handling |

Typical small deployment:

- 1 uWS main thread
- 1 WorkerThread
- 1 mediasoup worker process

## Startup Sequence

All critical modules are initialized before the server begins accepting traffic:

1. `WorkerThread::start()`: create worker processes
2. `waitReady()`: block until WorkerThreads report initialization complete
3. `uWS::App().listen()`: only then begin accepting WebSocket / HTTP connections

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
```

### Server PlainTransport

```text
external media ingress
      -> WebSocket join / plainPublish / plainSubscribe
      -> bind mediasoup UDP RTP/RTCP
      -> server creates PlainTransport / Producer / Consumer
      -> reuse RoomService and QoS aggregation paths
```

### Media

The media plane does not pass through the signaling logic after setup:

```text
Browser A ──SRTP/UDP──→ WebRtcTransport → Producer
                                            ├──→ Consumer (SIMPLE) → WebRtcTransport → Browser B
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

## Local Room Ownership

The runtime now operates in local-only mode:

- room ownership lives only in process memory
- `roomId -> WorkerThread` dispatch is local to the current node
- `/api/resolve` returns the local node for current deployments
- Redis is no longer required for room ownership, node cache, or pub/sub sync

## Geo Routing

If `GeoRouter` is enabled:

- client IP is mapped via `ip2region`
- same-country routing can be enforced
- candidate nodes are ranked by:
  - country isolation
  - ISP affinity
  - geographic distance
  - current load

This logic is provided by `GeoRouter`; in the current runtime it supports local node selection and registration metadata.

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
  --lat=30.27 \
  --lng=120.15 \
  --isp=电信 \
  --country=中国

# US West node
./build/mediasoup-sfu \
  --nodaemon \
  --port=3001 \
  --lat=37.39 \
  --lng=-122.08 \
  --isp=Amazon \
  --country="United States"
```

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
- `curl` and `tar` (used by `setup.sh` to fetch and unpack `mediasoup-worker`)

### Build

```bash
git clone https://github.com/user/mediasoup-cpp.git
cd mediasoup-cpp
# For this local-only test branch, do not run remote submodule update.
# Fill in third_party/*, src/mediasoup-worker-src, and ./mediasoup-worker from the local full source tree.
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
./build/mediasoup-sfu --nodaemon --port=3000
```

Recommended production-style invocation:

```bash
./build/mediasoup-sfu \
  --nodaemon \
  --port=3000 \
  --workers=1 \
  --workerThreads=1 \
  --workerBin=./mediasoup-worker \
  --hawkeyeRegisterUrl=ws://127.0.0.1:8080/register_ws \
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
  "hawkeyeRegisterUrl": "ws://127.0.0.1:8080/register_ws",
  "hawkeyeRegisterType": "mediasoup",
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
| `--workerBin` | `./mediasoup-worker` | worker binary path |
| `--logDir` | `/var/log/mediasoup` | daemon log directory |
| `--logPrefix` | `mediasoup-sfu` | daemon log file prefix |
| `--logLevel` | `info` | log verbosity |
| `--logRotateHours` | `3` | rotate daemon log every N hours into files like `mediasoup-sfu_2026041306_<pid>.log` (`0` disables rotation) |
| `--nodaemon` | flag | run in foreground |
| `--hawkeyeRegisterUrl` | empty | Hawkeye websocket registration endpoint, e.g. `ws://<hawkeye-host>:<port>/register_ws` |
| `--hawkeyeRegisterType` | `mediasoup` | Service type reported to Hawkeye |
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

### 1. Public IP auto-detection

The code auto-detects the public IP at startup. If detection fails, startup fails.

Set:

- optionally `--nodeId`
- optionally `--nodeAddress` if your environment has special routing or proxy topology
- set `--hawkeyeRegisterUrl=ws://<hawkeye-host>:<port>/register_ws` if this node should register itself with Hawkeye

### 2. Run tests from repo root

The integration tests spawn binaries using relative paths under `./build`.

### 3. Single-node still needs a reachable node address

The server now runs only in local single-node mode. `--nodeAddress` can still be set to publish an external address, but it is no longer used for room ownership routing.

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
./build/mediasoup_integration_tests
./build/mediasoup_qos_integration_tests
./build/mediasoup_e2e_tests
./build/mediasoup_bench
```

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
- geo routing
- country isolation

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

- default mode runs all default QoS groups, including server QoS, browser harnesses, and browser/downlink matrices; it continues even if one group fails
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
├── SignalingServerRuntime.cpp # runtime snapshot and room dispatch helpers
├── SignalingSocketState.h     # ws session / rate-limit helpers
├── SignalingRequestDispatcher.h # method -> RoomService dispatch glue
├── StaticFileResponder.h      # static-file path resolution + streaming
├── WorkerThread.*        # epoll event loop per signaling worker thread
├── RoomService.h         # room-service facade
├── RoomServiceLifecycle.cpp # join/leave/health/cleanup
├── RoomServiceMedia.cpp  # transport / produce / consume flows
├── RoomServiceStats.cpp  # stats / QoS / room-state broadcast
├── RoomServiceDownlink.cpp # downlink planning + publisher supply
├── RoomMediaHelpers.h    # media-side helper routines
├── RoomDownlinkHelpers.h # downlink helper routines
├── RoomStatsQosHelpers.h # stats/QoS helper routines
├── RoomManager.h         # room container and lifecycle
├── GeoRouter.h           # geolocation and scoring
├── WorkerManager.h       # worker selection / capacity helpers
├── Worker.*              # mediasoup-worker child process wrapper
├── Channel.*             # FlatBuffers IPC over Unix pipes
├── Router.*              # router wrapper
├── Transport.*           # transport wrapper
├── WebRtcTransport.*     # ICE / DTLS transport
├── PlainTransport.h      # plain RTP transport
├── Producer.*            # producer wrapper
├── Consumer.*            # consumer wrapper
├── Peer.h                # peer + session state
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
