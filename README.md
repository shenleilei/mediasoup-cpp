# mediasoup-cpp

A pure C++17 SFU (Selective Forwarding Unit) server that replaces mediasoup's Node.js control layer while keeping the battle-tested mediasoup-worker for media processing.

## Why

mediasoup is the most widely used open-source SFU, but its control layer is Node.js. This project replaces it with C++17 for:

- **Lower latency** — no V8/libuv overhead on the signaling path
- **Single binary deployment** — no Node.js runtime dependency
- **Predictable performance** — no GC pauses

The mediasoup-worker C++ binary (media processing) is used as-is, communicating via Unix pipe + FlatBuffers.

## Architecture

```
Browser A ──WebSocket──→ SignalingServer (uWS, non-blocking)
                              │ postWork()
                              ▼
                         Worker Thread (serial task queue)
                              │
                         RoomService (business logic)
                              │
                         Router → Transport → Producer/Consumer
                              │
                         Channel (pipe + FlatBuffers)
                              │
                    ──────────┼────────── process boundary
                              │
                    mediasoup-worker (prebuilt v3.14.16)
                              │
                         RTP forwarding (SRTP, NACK, BWE, Simulcast)
                              │
Browser B ←──SRTP/UDP────────┘
```

## Features

- **Room-first design** — auto-subscribe: when a peer produces, all other peers in the room automatically receive the stream
- **Non-blocking signaling** — all Channel IPC calls run on a dedicated worker thread; the uWS event loop never blocks
- **H264/VP8 recording** — RTP depacketization, frame reassembly, FFmpeg muxer to WebM, with synchronized QoS timeline
- **QoS monitoring** — real-time stats panel (bitrate, jitter, RTT, packet loss, score) with 3-second refresh, verified against tcpdump
- **Multi-node routing** — Redis-based room registry with dead-node takeover and automatic redirect
- **Worker crash recovery** — automatic respawn with rate limiting, client notification within 2 seconds
- **Daemon mode** — fork + setsid, structured logging, PID file, auto public IP detection

## Quick Start

### Prerequisites

- Linux (tested on Ubuntu 20.04+, Debian 11+)
- CMake 3.16+, GCC 10+ or Clang 12+
- OpenSSL, zlib, FFmpeg (libavformat, libavcodec, libavutil)
- hiredis (optional, for multi-node Redis routing)

### Build

```bash
git clone --recursive https://github.com/user/mediasoup-cpp.git
cd mediasoup-cpp
./setup.sh              # downloads mediasoup-worker binary + installs deps
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### Run

```bash
# Foreground with defaults
./build/mediasoup-sfu --nodaemon --port=3000 --listenIp=0.0.0.0

# With config file
cat > config.json << EOF
{
  "port": 3000,
  "workers": 0,
  "workerBin": "./mediasoup-worker",
  "listenIp": "0.0.0.0",
  "announcedIp": "",
  "redisHost": "127.0.0.1",
  "redisPort": 6379
}
EOF
./build/mediasoup-sfu
```

Open `http://<server-ip>:3000` in two browser tabs to start a video call.

### Command-line Options

| Option | Default | Description |
|--------|---------|-------------|
| `--port` | 3000 | Signaling + HTTP port |
| `--workers` | CPU count | Number of mediasoup-worker processes |
| `--listenIp` | 0.0.0.0 | WebRTC listen IP |
| `--announcedIp` | auto-detect | Public IP for ICE candidates |
| `--workerBin` | ./mediasoup-worker | Path to worker binary |
| `--recordDir` | ./recordings | Recording output directory |
| `--nodaemon` | (flag) | Run in foreground |
| `--redisHost` | 127.0.0.1 | Redis host (multi-node) |
| `--nodeId` | auto | Node identifier |

## Thread Model

| Thread | Count | Role |
|--------|-------|------|
| uWS main | 1 | Event loop: WebSocket, HTTP, timers (never blocks) |
| Signaling worker | 1 | Serial execution of RoomService methods + Channel IPC |
| Channel reader | ×N workers | Reads pipe responses/notifications from worker |
| Worker waiter | ×N workers | waitpid(), respawns on crash |
| Recorder | ×M peers | UDP recv → RTP depacketize → FFmpeg mux |

Typical: 1 worker + 2 recording peers = **5 threads**.

## Data Flow

### Signaling (e.g. produce request)

```
Browser → WS message → uWS main thread (JSON parse, postWork)
  → Worker thread: RoomService.produce()
    → ORTC negotiation
    → Channel.RequestWait() → pipe write → worker processes → pipe read
    → Auto-subscribe: create Consumer for each other peer
    → Auto-record: PlainTransport + PIPE Consumer
  → Loop::defer() → uWS main thread → ws.send(response)
  → Loop::defer() → ws.send(newConsumer notification) → other browsers
```

### Media (RTP, does not pass through control layer)

```
Browser A ──SRTP/UDP──→ WebRtcTransport → Producer
                                            ├──→ Consumer (SIMPLE) → WebRtcTransport → Browser B
                                            └──→ Consumer (PIPE) → PlainTransport
                                                    │ plaintext RTP, UDP 127.0.0.1
                                                    ▼
                                              PeerRecorder → .webm file
```

## Testing

```bash
# Unit tests (98 tests)
./build/mediasoup_tests

# Integration tests (requires built mediasoup-sfu + worker)
./build/mediasoup_integration_tests        # 14 tests
./build/mediasoup_stability_integration_tests  # 10 tests
./build/mediasoup_qos_integration_tests    # 15 tests
./build/mediasoup_e2e_tests                # 3 tests
./build/mediasoup_topology_tests           # 6 tests

# Worker load benchmark
./build/mediasoup_bench
```

## Performance

Tested on Intel Xeon Platinum 2.5GHz, 2 vCPU:

| Metric | Loopback | Real Network |
|--------|----------|--------------|
| Peak rooms (1P+2C each) | 240 | 80 |
| Worker CPU | 82% (single core) | 23% |
| RSS | 180 MB | 67 MB |
| PPS (in→out) | 72k→144k | 24k→48k |

Real-world estimate (WebRTC + SRTP + audio + video): **~30-40 1v1 rooms per worker**.

Network bottleneck on virtio single-queue VMs is kernel softirq, not worker CPU. Multi-queue NICs or bare metal eliminate this.

## Project Structure

```
src/
├── main.cpp              # Entry point, config parsing
├── Constants.h           # All magic numbers as named constants
├── SignalingServer.h/cpp # WebSocket + HTTP + worker thread
├── RoomService.h/cpp     # Business logic (join/leave/produce/consume/QoS/recording)
├── RoomManager.h         # Room/Peer lifecycle, idle GC
├── RoomRegistry.h        # Redis multi-node routing
├── WorkerManager.h       # Worker pool, load balancing, crash recovery
├── Channel.h/cpp         # Pipe IPC (FlatBuffers, request/response/notification)
├── Worker.h/cpp          # fork+exec worker process management
├── Router.h/cpp          # Router: create transports, manage producers
├── Transport.h/cpp       # Produce/Consume via FlatBuffers protocol
├── WebRtcTransport.h/cpp # ICE/DTLS transport
├── PlainTransport.h      # Plain RTP transport (recording)
├── Producer.h/cpp        # Media producer (score tracking, stats)
├── Consumer.h/cpp        # Media consumer (score tracking, stats)
├── ortc.h                # ORTC negotiation (codec matching, RTP mapping)
├── Recorder.h            # RTP→WebM recording + QoS timeline
├── EventEmitter.h        # Lightweight event system
└── Logger.h              # spdlog wrapper
tests/
├── test_ortc.cpp         # ORTC negotiation (25 tests)
├── test_room.cpp         # Room/Peer management
├── test_stability.cpp    # Crash recovery, EventEmitter cleanup
├── test_worker_queue.cpp # Worker thread FIFO, alive token
├── test_integration.cpp  # Black-box: real SFU + WebSocket clients
├── test_e2e_pubsub.cpp   # Multi-peer publish/subscribe stress
└── bench_worker_load.cpp # Worker capacity benchmark
public/
├── index.html            # Video call page with QoS monitor panel
└── playback.html         # Recording playback with QoS timeline
```

## License

MIT — see [LICENSE](LICENSE).

## Acknowledgments

- [mediasoup](https://mediasoup.org/) — the SFU worker and protocol design
- [uWebSockets](https://github.com/uNetworking/uWebSockets) — HTTP + WebSocket server
- [FlatBuffers](https://google.github.io/flatbuffers/) — IPC serialization
- [nlohmann/json](https://github.com/nlohmann/json) — JSON for C++
- [spdlog](https://github.com/gabime/spdlog) — logging
- [FFmpeg](https://ffmpeg.org/) — recording muxer
