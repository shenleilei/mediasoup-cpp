# 运行时安全规范 (Runtime Safety)

## WebSocket 信令层 (WebSocket Signaling)

- 格式错误的 WebSocket 请求负载（Payload）**必须 (SHALL)** 被直接丢弃，并且**严禁 (SHALL NOT)** 导致信令服务器进程崩溃。
- 格式错误的请求**严禁 (SHALL NOT)** 污染当前连接状态，也不能阻止后续有效请求的正常处理。
- 单个 WebSocket 连接**严禁 (SHALL NOT)** 建立多个加入（joined）的会话；在已加入状态的 Socket 上重复发送 `join` 请求**必须 (SHALL)** 被拒绝。
- 如果 WebSocket 在信令循环中的异步 `join` 尚未完成前断开连接，当该 Peer 的会话 ID 仍匹配时，服务器**必须 (SHALL)** 在 Worker 侧执行回滚（Rollback）操作并清理该 Peer。
- 当异步 `join` 已在 Worker 侧成功，但主线程完成回调发现目标 Worker 已不可用时，服务器**严禁 (SHALL NOT)** 提交 joined socket 会话状态；系统**必须 (SHALL)** 清理 pending join 状态，并向仍存活的 Socket 返回显式失败。

## 房间注册中心排序 (Room Registry Ordering)

- 当缺乏地理位置（Geo）数据时，房间注册中心的候选节点排序**必须 (SHALL)** 优先选择当前房间数较少的节点。
- 当房间数相同时，当前节点可以作为具有确定性的最终平局决胜条件被优先考虑。
- 无地理位置数据的比较器**必须 (SHALL)** 满足严格弱序（Strict Weak Ordering）。

## Worker 与 Channel 关闭控制 (Worker And Channel Shutdown)

- Worker 的关闭以及 Worker 死亡的处理路径**必须 (SHALL)** 是幂等的（Idempotent），并且在跨线程调用时必须保证线程安全。
- Worker 的启动阶段**必须 (SHALL)** 避免在 `fork()` 后执行非异步信号安全（non-async-signal-safe）的初始化设置，且在失败时**必须 (SHALL)** 妥善清理部分创建的管道资源。
- 当 `Channel::close()` 在读取线程内部自身被调用时，**严禁 (SHALL NOT)** 遗留未被回收（joinable）的读取线程。
- 在并发关闭期间，`Channel` 的发送与关闭路径**严禁 (SHALL NOT)** 向已经失效的 Producer 文件描述符进行写入。
- `Channel` 的 pipe 写入路径**必须 (SHALL)** 重试可恢复的中断错误（例如 `EINTR`），而**严禁 (SHALL NOT)** 仅因该类可恢复错误就关闭整个 Channel。
- 非线程化（Non-threaded）的 `Channel` 消息抽取器**必须 (SHALL)** 能够容忍重入 `requestWait()` 的通知回调，且不会重复执行（Replay）同一条缓冲消息。
- `WorkerThread` 事件循环**必须 (SHALL)** 显式处理 `epoll_wait` 异常；应当重试 `EINTR` 错误，对于不可恢复的严重错误应当记录日志并打破循环，以避免陷入静默降级的死循环中。
- 纯 C++ 客户端 `NetworkThread` 的关闭路径**必须 (SHALL)** 有界完成；即使 legacy pacing 队列仍有残留包或当前 pacing budget 无法推进，关闭流程也**严禁 (SHALL NOT)** busy-spin。
- 当纯 C++ 客户端关闭 transport controller、退回 legacy pacing fallback 时，已注册的视频 track **必须 (SHALL)** 具备非零的默认 pacing 目标码率，使其在显式 transport hint 到来前仍可发送 RTP。

## 运维日志 (Operational Logging)

- 正常的 `/api/resolve` 和房间注册中心维护追踪记录**必须 (SHALL)** 记录在 `Warning` 级别以下；`Warning` (警告) 级别日志仅保留给故障、降级或异常状态。

## 运行时就绪状态 (Runtime Readiness)

- 基于 Redis 的房间注册中心默认**必须 (SHALL)** 被视为就绪（Readiness）的依赖项。
- 当要求必须使用 Redis 但其在启动时不可用，进程**必须 (SHALL)** 显式返回启动失败，而不是静默降级为仅本地路由（local-only）模式。
- 当要求必须使用 Redis 但其在运行时变为不可用，`/readyz` 接口**必须 (SHALL)** 返回 `503`，并且新的依赖路由的请求**必须 (SHALL)** 显式失败，而不是退化到仅本地路由。

## Worker 崩溃处理 (Worker Crash Handling)

- 当 mediasoup worker 崩溃导致现有房间不可继续服务时，服务器**必须 (SHALL)** 向受影响房间内的客户端发送 `serverRestart` 通知。
- 当前系统认可的崩溃处理模型是“通知并拆除旧房间，再允许新房间在 respawn 后的 worker 上继续创建”；系统**不承诺 (DOES NOT GUARANTEE)** 旧房间的无损状态恢复。

## Transport Connect Contract

- `WebRtcTransport::connect()`、`PlainTransport::connect()` 与 `PipeTransport::connect()` **必须 (SHALL)** 校验 Worker 返回的响应体类型与所请求的 connect 操作一致。
- 当 connect 响应体缺失、类型不匹配、或缺少必需字段（例如 DTLS local role 或 transport tuple）时，这些 helper **严禁 (SHALL NOT)** 返回假成功，而**必须 (SHALL)** 显式失败。

## RTP 与能力正确性 (RTP And Capability Correctness)

- `repaired-rtp-stream-id` 头部扩展（Header Extension）**必须 (SHALL)** 映射到 repair-stream URI 的枚举值，而不是普通的 stream-id 枚举值。
- 在宣告支持的 RTP 能力中，**严禁 (SHALL NOT)** 为不同的编解码器（Codec）变体重复使用相同的非零 Payload Type。

## 输入淬火与安全防护 (Input Hardening)

- `/api/resolve` 接口与 WS `join` 握手**严禁 (SHALL NOT)** 盲目信任用户直接提供的 IP 查询参数或 JSON 字段来做地理路由（Geo-routing）决策。系统**必须 (MUST)** 从底层的 Socket 远端地址，或者从被明确验证过的反向代理 `X-Forwarded-For` HTTP 头中提取用户位置。
- 纯 C++ 客户端的 `SourceWorker` 在文件输入或摄像头输入路径上，如果 FFmpeg decoder 参数导入或 `avcodec_open2()` 失败，**必须 (SHALL)** 安全停止，且**严禁 (SHALL NOT)** 继续进入后续 decode/send 循环。
- 无效的数字型 CLI 参数**必须 (SHALL)** 在启动时显式抛出错误并退出，而不能直接 Crash 或静默回退到默认值。
