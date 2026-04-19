# 非安全问题Review汇总（mediasoup-cpp）

> 范围说明：本文件仅汇总“非安全问题”（正确性、稳定性、并发、资源管理、性能、可维护性、可移植性、错误处理等）。
> 
> 不包含：认证/鉴权、注入、越权、DoS、防护策略、CORS、MITM、SSRF 等安全类问题。

## 1. 关键问题（Critical）

1. **RTP 扩展 URI 匹配顺序错误导致语义错配**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/RtpParametersFbs.h`（约 78-82）  
   - 现象：`repaired-rtp-stream-id` 被 `rtp-stream-id` 子串规则提前命中。  
   - 影响：RTP 修复流相关能力被错误映射，可能造成媒体协商与恢复逻辑异常。

2. **Payload Type 重复定义冲突**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/supportedRtpCapabilities.h`（约 38、52）  
   - 现象：PT 108 被两个不同能力复用。  
   - 影响：编解码/重传协商冲突，可能导致媒体解读错误或失败。

3. **RoomService 异步任务捕获裸 `this` 存在生命周期风险**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/RoomServiceStats.cpp`（约 480）、`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/RoomServiceDownlink.cpp`（约 28）  
   - 现象：投递延迟/异步任务时使用 `[this]`。  
   - 影响：对象销毁后任务仍执行会触发悬空引用（UAF）。

4. **fork 后 exec 前调用非 async-signal-safe 逻辑**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/Worker.cpp`（约 101-120）  
   - 现象：子进程在 `fork()` 后 `execvp()` 前执行 `std::vector`/`std::to_string`/`setenv` 等。  
   - 影响：多线程场景下可能死锁或触发未定义行为。

## 2. 高优先级问题（High）

1. **Channel 发送路径存在 fd 检查与写入竞态（TOCTOU）**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/Channel.cpp`（约 120-132）  
   - 影响：并发 close/send 时可能写到无效或复用 fd，造成异常行为。

2. **Channel 接收处理可重入，可能导致缓冲区状态错乱**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/Channel.cpp`（约 323-387）  
   - 影响：重复处理、偏移失效甚至越界读取风险。

3. **Producer/Consumer/Router 使用 Channel 裸指针，生命周期边界脆弱**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/Producer.h`、`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/Consumer.h`、`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/Router.h`  
   - 影响：当 Worker/Channel 先销毁时，后续调用可能触发悬空访问。

4. **pipe 创建异常路径 fd 泄漏**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/Worker.cpp`（约 73-75）  
   - 影响：部分失败分支未关闭已创建 fd，长期运行累积资源泄漏。

5. **Redis 回复解析缺少元素类型/空指针防护**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/RoomRegistry.cpp`（约 511-514、631-634）  
   - 影响：在异常回复下可能触发空指针构造字符串导致崩溃。

6. **CLI 参数解析异常未兜底**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/MainBootstrap.cpp`（约 159-183）  
   - 影响：`std::stoi/std::stod` 抛异常可直接中止进程。

## 3. 中优先级问题（Medium）

1. **`Room::touch()` 与读取路径锁策略不一致，存在数据竞争风险**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/RoomManager.h`（约 86）

2. **Room 与 Peer 并发模型不统一**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/Peer.h`、`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/RoomManager.h`  
   - 现象：Room 层有锁，Peer 内部可变状态普遍无锁。  
   - 影响：跨线程读写时有数据竞争隐患。

3. **`Room::removePeer()` 持锁调用 `peer->close()` 可能放大死锁链路**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/RoomManager.h`（约 51-59）

4. **`firstRtpTime_` 在不同互斥量下读写**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/Recorder.h`（约 388-392）  
   - 影响：时间戳统计路径可能出现数据竞争。

5. **`WorkerManager::maxRoutersPerWorker_` 读写锁策略不一致**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/WorkerManager.h`

6. **`epoll_ctl` / `write(eventfd)` 返回值未充分检查**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/WorkerThread.cpp`（约 167、226、429）  
   - 影响：失败后进入“静默异常状态”，任务唤醒/事件监听失效难排查。

7. **`ortc.h` 头文件内 `static` 容器导致多 TU 副本**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/ortc.h`（约 12）  
   - 影响：不必要内存与初始化开销。

8. **延迟任务队列 `std::move(const&)` 实际未移动**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/WorkerThread.cpp`（约 344）  
   - 影响：闭包对象退化为拷贝，增加开销。

9. **计时器资源回收不完整**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/SignalingServerHttp.cpp`（约 238-298）  
   - 影响：退出/重建场景下残留资源。

10. **MIME 类型判断采用子串匹配，扩展名判断不严谨**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/StaticFileResponder.h`（约 80-86）

11. **GeoRouter 可拷贝语义未显式禁用**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/GeoRouter.h`  
   - 影响：资源所有权对象若被误拷贝，存在重复释放风险。

12. **`geo_` 指针访问同步策略不统一**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/RoomRegistry.h`、`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/RoomRegistry.cpp`  
   - 影响：并发更新/读取可能出现竞态。

## 4. 低优先级问题（Low）

1. **`signal()` 可移植性与语义一致性弱于 `sigaction()`**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/main.cpp`

2. **日志级别偏高导致噪声**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/SignalingServerRuntime.cpp`  
   - 影响：正常路径大量 warn 不利于故障定位。

3. **`catch (...)` 直接吞异常导致可观测性差**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/SignalingServerRuntime.cpp`、`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/RoomService.h`

4. **`gethostname` 截断边界处理不严谨**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/MainBootstrap.cpp`

5. **FFmpeg 部分接口已过时**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/Recorder.h`

6. **部分时间相关逻辑同函数内多次取时，易引入微小抖动**  
   - 文件：`/home/runner/work/mediasoup-cpp/mediasoup-cpp/src/RoomServiceStats.cpp` 等

## 5. 非安全问题修复建议（执行顺序）

1. **先修协议正确性（P0）**：RTP extension 映射顺序、PT 冲突。  
2. **再修生命周期与并发（P0/P1）**：`[this]` 异步捕获、Channel fd 竞态、裸指针所有权。  
3. **补齐异常与系统调用兜底（P1）**：CLI 解析异常、`epoll_ctl/write` 返回值、Redis reply 类型校验。  
4. **治理资源与性能（P2）**：fd/timer 回收、重入路径、无效 move、头文件静态容器。  
5. **最后做可维护性清理（P3）**：日志分级、过时 API、边界与注释一致性。

## 6. 备注

- 本文基于当前仓库代码审阅结论整理，定位以“文件 + 近似行号”为主。  
- 若进入修复阶段，建议按模块拆分小 PR，并为每一类问题补对应回归测试。
