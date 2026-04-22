# mediasoup-worker 升级评估与问题汇总

## 1. 文档目的

这份文档把当前仓库关于 `mediasoup-worker` 升级的关键问题集中记录下来，避免未来再从聊天记录里重复重建上下文。

它回答的不是“worker 内部架构是什么”，而是：

1. 当前仓库到底基于哪个 worker 基线。
2. 本地已经带了哪些 worker 补丁。
3. 最近上游 release 新增了什么、修了什么。
4. 哪些修复值得优先 backport。
5. 如果直接升到较新的 release，会先撞上哪些协议和构建问题。

配套的 worker 架构说明见：

- [mediasoup-worker-architecture-analysis_cn.md](./mediasoup-worker-architecture-analysis_cn.md)

## 2. 当前本地基线

### 2.1 子模块基线

当前仓库的 vendored worker 源码位于：

- `src/mediasoup-worker-src`

它不是纯 `3.14.6`，而是：

- 上游 tag：`3.14.6`
- tag commit：`c042ee7f08ca94ac903f8140f66febb720ef9f91`
- 当前子模块 HEAD：`450d866037183b36cb1f9d1d8e3379e219930c15`

也就是说，当前本地 worker 基线是：

- `3.14.6 + 1 个本地提交`

本地额外提交是：

- `450d866 Fix local worker build inputs and mirror sources`

### 2.2 运行时接入方式

本仓库当前不是把 worker 代码静态链接进主程序，而是以外部进程方式启动 worker。

关键点：

- 默认 worker 路径是 `./mediasoup-worker`
- 运行时通过 `--workerBin` 覆盖
- `src/Worker.cpp` 里通过 `posix_spawnp()` 拉起子进程

相关位置：

- [README.md](/root/mediasoup-cpp/README.md)
- [src/Worker.cpp](/root/mediasoup-cpp/src/Worker.cpp)

这意味着：

- 可以继续以上游为基线
- 也可以在本地编译自定义 worker 二进制并通过 `--workerBin` 切换
- 但只要升级 worker 二进制，FBS/IP​​C 协议仍必须与控制面保持兼容

## 3. 当前本地 worker 补丁

当前本地 worker 提交 `450d866` 实际只改了 4 个文件：

- `worker/subprojects/libuv.wrap`
- `worker/subprojects/openssl.wrap`
- `worker/meson.build`
- `worker/deps/libwebrtc/meson.build`

可以归纳成两类。

### 3.1 镜像补丁

目的：

- 优先使用 Aliyun 镜像下载第三方依赖

当前已做：

- `openssl.wrap`：把 `source_url` 指向 Aliyun 镜像，`source_fallback_url` 指回官方源
- `libuv.wrap`：对 `3.14.6` 使用了一个可用的 Aliyun 镜像 URL

当前观察：

- `openssl-3.0.8` 的 Aliyun URL 仍可用
- `libuv-v1.48.0` 的旧 Aliyun URL 可用
- 但新 upstream `3.19.21` 使用的 `libuv-v1.51.0`，尚未找到当前可用的 Aliyun URL

这意味着：

- OpenSSL 镜像补丁大概率可以继续带
- libuv 镜像补丁不能机械套到新版本，必须先重新验证制品是否存在

### 3.2 本地构建补丁

目的：

- 把 `flatbuffers_generator` 直接加入若干 Meson 目标的 `sources`

当前已做：

- 在 `worker/meson.build` 中把 `flatbuffers_generator` 加进 `libmediasoup-worker`、`mediasoup-worker`、`mediasoup_worker_test` 等目标
- 在 `worker/deps/libwebrtc/meson.build` 中也把它并入 `libwebrtc` 的 sources

当前判断：

- 这更像“本地直跑 Meson 构建时的兜底补丁”
- 不像“升级到新 release 后必须长期携带的功能补丁”
- 因为 upstream `tasks.py` 本来就会先执行 `flatc` / `flatbuffers-generator`，再编译 worker

因此：

- 如果继续沿用 upstream 的 `invoke mediasoup-worker` 构建路径，这个补丁大概率可以重新验证后再决定是否保留
- 如果要继续支持本地绕过 upstream 构建脚本、直接执行 Meson 目标，则需要重新验证这个补丁在新版本是否仍有必要

## 4. 最近上游 release 发生了什么

这里关注的是与当前仓库直接相关的 worker 变化，不是完整 changelog 转抄。

### 4.1 `3.19.21`（2026-04-21）

- 修复 `DirectTransport` 在关闭 `DataProducer` / `DataConsumer` 时的回归

判断：

- 是 bugfix
- 对当前仓库的优先级较低，因为当前非 vendored 代码里没有明显业务路径在使用 `DirectTransport`

### 4.2 `3.19.20`（2026-04-20）

- 新增 `useBuiltInSctpStack` 配置项，默认 `false`

判断：

- 这是新选项，不是默认行为切换
- 更像是后续 SCTP 内建路径的铺垫
- 对当前仓库的直接收益不高

### 4.3 `3.19.19`（2026-03-24）

- 接收 buffer 改成 4-byte 对齐，避免 undefined behavior
- `liburing` 从 `2.12-1` 升级到 `2.14-1`

判断：

- 第一条是高价值稳定性修复
- 第二条是底层依赖更新，价值依赖现网环境和 `liburing` 使用情况

### 4.4 `3.19.18`（2026-03-13）

- `Utils::Crypto::GetRandomUInt()` 改进
- `WORKER_CLOSE` 从 request 改成 notification
- 要求 C++20
- 修复 `enableSctp: true` 但未创建 DataChannel 时的 `SCTP failed`

判断：

- 这版对当前仓库很敏感
- 其中真正“值钱”的 bugfix 是 SCTP 无 DataChannel 修复
- 但同版还包含协议和构建层破坏性变化，所以直接整版升级成本很高

### 4.5 `3.19.17`（2026-02-05）

- 修复 `ICE::StunPacket::GetXorMappedAddress()` 的错误内存访问

判断：

- 这是通用的稳定性/内存安全修复
- 优先级较高

### 4.6 `3.19.15`

- `RtpStreamSend`: duplicated packets are discarded
- worker 内部开始切换到新的 `RTP::Packet` / `ICE::StunPacket` 结构
- `liburing` 升级

判断：

- 真正高价值的是“重复 RTP 包不再继续发送”的那个 bugfix
- 但整个 `3.19.15` release 夹带了大量内部重构
- 因此更适合把关键 bugfix 单独 backport，而不是把整版一起抬进来

### 4.7 `3.19.9`

- 修复 RTCP `packets_lost` 相关统计错误

判断：

- 这条对当前仓库非常关键
- 它不只是“看板数字修复”，还会影响发送侧对 RTCP Receiver Report 的解释

## 5. 当前最值得优先吸收的 bugfix

如果按当前仓库的实际价值排序，优先级建议是：

### P0: `3.19.9` 的 RTCP `packets_lost` 修复

原因：

- 本仓库非常依赖 QoS / stats / 回归报告
- `packets_lost` 类型错误会直接污染统计口径
- 发送侧也会读取该值参与 loss / score 相关判断

优先结论：

- 高优先级 backport 候选

### P0: `3.19.15` 的 duplicated RTP packet discard

原因：

- 位于发送热路径
- 会直接影响重复包、重传、弱网场景的行为
- 提交本身相对集中，适合独立回填

优先结论：

- 高优先级 backport 候选

### P1: `3.19.17` 的 STUN 内存访问修复

原因：

- 属于通用稳定性修复
- 影响 WebRTC 入网基本路径

### P1: `3.19.19` 的接收 buffer 4-byte 对齐

原因：

- 属于 UB 修复
- 风险在于“现网不一定稳定复现，但出问题时不容易定位”

### P2: `3.19.18` 的 SCTP no-DataChannel 修复

原因：

- 如果当前不依赖 DataChannel/SCTP，优先级可下调
- 如果业务后续要用 SCTP，这条应前移

### P3: `3.19.21` 的 DirectTransport 回归修复

原因：

- 当前仓库业务层没有明显依赖 `DirectTransport`
- 可以留到真正涉及 DirectTransport 路径时再考虑

## 6. `3.19.9` 到底修复了什么

这条修复最容易被标题误导。

它不是在修“RTCP 包丢了”，而是在修：

- RTCP Receiver Report 里的 `total lost`
- 在 worker 内部和导出 stats 时被错误地按无符号数传播

### 6.1 字段语义

RTCP Receiver Report 的 `total lost` 按协议是：

- 24-bit signed value

也就是说它的正确语义不是“永远递增的无符号累计丢包数”，而是：

- `cumulative_lost = expected - received`

这里：

- `expected`：按 RTP sequence number 区间推导出的理论包数
- `received`：实际已接收包数

所以这个值是可以下降的，极端时也可以小于 0。

### 6.2 在 worker 里的实现路径

#### `expected`

在 `RtpStream` 中：

- `GetExpectedPackets()` 定义为
  `(cycles + maxSeq) - baseSeq + 1`

见：

- `worker/include/RTC/RtpStream.hpp`

这里依赖 `UpdateSeq()` 维护：

- `maxSeq`
- `cycles`
- `baseSeq`

见：

- `worker/src/RTC/RtpStream.cpp`

所以 `expected` 本质上是：

- 已观察到的 sequence 区间宽度

#### `received`

在 `RtpStreamRecv::GetRtcpReceiverReport()` 中，接收数来自：

- `mediaTransmissionCounter.GetPacketCount()`

见：

- `worker/src/RTC/RtpStreamRecv.cpp`

而 `mediaTransmissionCounter.Update(packet)` 的实现只是：

- `packets++`

见：

- `worker/src/RTC/RateCalculator.cpp`

这说明：

- `received` 统计的是被接纳进入计数器的包事件数
- 不是 sequence 去重后的“唯一 RTP 包数”

### 6.3 为什么会出现负数或下降

因为 `expected` 和 `received` 不是同一维度。

会让 `expected - received` 下降的常见情况包括：

- 晚到包或可接受乱序包补回
- 重复包被接收计数器继续累计

例如：

- 按 sequence 区间推出来 `expected = 100`
- 但实际计数器已经累计 `received = 102`
- 那么 `lost = -2`

即使当前代码在某些局部会把即时 `packetsLost` 钳到不小于 0，累计导出链路仍然必须使用 signed 类型，因为“本次比上次更少”的 delta 是完全合法的。

### 6.4 `3.19.9` 的具体修复点

上游把这些位置从无符号改成了有符号：

- `worker/fbs/rtpStream.fbs`
  `packets_lost: uint64 -> int32`
- `RtpStream::packetsLost`
- `RtpStreamRecv::reportedPacketsLost`
- `RtxStream::packetsLost`
- `RtxStream::reportedPacketsLost`
- `RtpStreamSend` 读取 `report->GetTotalLost()` 时也按 signed 处理

这条修复的意义是：

- 防止负值在无符号链路里下溢
- 防止 stats 里出现离谱的大正数
- 防止发送侧 score / loss 相关逻辑被错误值污染

### 6.5 对当前仓库的影响

当前仓库自己的 `fbs/rtpStream.fbs` 仍然是旧定义：

- `packets_lost: uint64`

而本仓库也有自己的 stats 映射代码：

- `src/RtpStreamStatsJson.h`

这意味着：

- 如果直接换上带 `3.19.9` 语义的新 worker，而不同步本仓库顶层 `fbs/` 和 stats 映射，统计解释会出现不一致
- 如果不升级 worker，则当前仓库仍可能保留旧 signedness 问题

## 7. `3.19.15` 的重复 RTP 包修复到底修了什么

上游提交：

- `d986d5d11 Worker: RtpStreamSend, do not send duplicated RTP packets`

修复前：

- 发送侧如果开启重传缓冲，会先把包写入 retransmission buffer
- 但没有在热路径上先判断“这个 sequence 是否已经存在于缓冲里”

修复后：

- 在 `RtpStreamSend::ReceivePacket()` 中，先查 retransmission buffer
- 如果这个 sequence 已经存在，则直接 `DISCARDED`
- 不再继续把重复 RTP 包发出去

上游给出的原因很直接：

- 重复包继续发出去会让接收端解密失败
- 还会被 RTCP transport feedback 当成未正确接收，进一步影响带宽估计

为什么这条对当前仓库重要：

- 本仓库在弱网、重传、QoS、发送路径压测上投入很重
- 这条修复位于真实媒体热路径
- 它比很多“文档上看着高级”的新特性更有现网价值

为什么建议单独 backport，而不是整版抬 `3.19.15`：

- `3.19.15` 附近还包含新的 `RTP::Packet` / `ICE::StunPacket` 结构重构
- 整版升级会把大量内部目录和类型改名一起带进来
- 单个 bugfix 本身的收益和风险比更高

## 8. 直接升级到 `3.19.x` 会先撞上的问题

### 8.1 `WORKER_CLOSE` 从 request 改成 notification

上游在 `3.19.18` 把：

- `WORKER_CLOSE`

从 `request.fbs` 挪到了 `notification.fbs`。

这对当前仓库是直接破坏性的，因为本仓库仍按旧协议工作：

- `src/Worker.cpp` 里 `close()` 仍发送 `FBS::Request::Method::WORKER_CLOSE`
- 顶层 `fbs/request.fbs` / `fbs/notification.fbs` 仍是旧布局
- `tests/test_review_fixes.cpp` 也有旧 request 语义的测试

风险不只是一处方法名不匹配，而是：

- request 枚举值整体前移
- notification 枚举值也发生变化
- 如果只替换 worker 二进制，不同步本仓库顶层 FBS，控制面和 worker 会按不同编号解释同一条消息

### 8.2 顶层 FBS 已经整体漂移

从 `3.14.6` 到 `3.19.21`，不只是：

- `request.fbs`
- `notification.fbs`

发生了变化，至少还有：

- `rtpParameters.fbs`
- `rtpStream.fbs`
- `transport.fbs`

也变了。

这些变化会继续影响：

- `src/RtpParametersFbs.h`
- `src/RtpStreamStatsJson.h`
- `src/Transport.cpp`

所以：

- 升级到 `3.19.x` 不是“改一个 close 语义”就结束
- 而是需要把本仓库顶层 `fbs/` 和相关编解码逻辑整体对齐一轮

### 8.3 构建门槛变化

从 `3.19.x` 开始，worker 构建链也明显抬高了门槛：

- worker 需要 `C++20`
- upstream `worker/meson.build` 要求 `Meson >= 1.1.0`

当前本地环境观察到：

- 系统 `meson` 是 `0.61.5`

也就是说：

- 即使 FBS/IP​​C 层完全对齐
- 当前环境也不能直接用旧 Meson 路径跑新版 worker build

## 9. 当前本地 patch 在新版本上的延续建议

### 9.1 明确保留：OpenSSL 镜像补丁

原因：

- 当前 upstream 仍是 `openssl-3.0.8`
- Aliyun 镜像 URL 当前可用
- 符合仓库“优先 Aliyun，缺失时 fallback upstream”的规则

### 9.2 重新设计：libuv 镜像补丁

原因：

- upstream 已升级到 `libuv-v1.51.0`
- 当前还没有确认可用的 Aliyun 镜像制品
- 旧 `1.48.0` 的镜像补丁不能直接套到新版本

建议：

- 不要先写死一个未经验证的新镜像 URL
- 升级记录里明确记下“当前未找到镜像，暂时使用 upstream fallback”

### 9.3 待验证：flatbuffers / Meson 本地构建补丁

原因：

- 当前补丁更像对本地构建方式的兜底
- upstream 自带 `tasks.py` 已经把 `flatc` / `flatbuffers-generator` 放在默认构建序列里

建议：

- 如果后续仍用 upstream `invoke mediasoup-worker`，优先尝试不带这组补丁
- 如果后续继续支持 repo 内部“直跑 Meson”的构建姿势，再单独验证是否需要恢复

## 10. 建议的近期路线

### 10.1 如果目标是“先拿值钱的修复”

优先考虑 targeted backport：

1. `3.19.9` 的 `packets_lost` signedness/statistics 修复
2. `3.19.15` 的 duplicated RTP packet discard 修复
3. `3.19.17` 的 STUN 内存访问修复
4. `3.19.19` 的接收 buffer 对齐修复

这样做的优点：

- 不必立刻吞下 `3.19.18+` 的 FBS 协议漂移
- 不必立刻切构建链到新版 Meson/C++20
- 能先吸收对当前仓库最值钱的媒体面修复

### 10.2 如果目标是“正式升到较新的 worker release”

建议先把下面这些作为前置任务，而不是边升级边碰运气：

1. 同步顶层 `fbs/` 到目标 release 的 worker FBS
2. 同步 `RtpParametersFbs` / `RtpStreamStatsJson` / `Transport` 等编解码桥接层
3. 改造 `WORKER_CLOSE` 调用路径为 notification 语义
4. 确认本地 worker 构建链支持 `C++20` 和新的 Meson 基线
5. 重新验证本地镜像补丁是否仍成立

## 11. 结论

当前最重要的判断不是“上游最近有没有新 feature”，而是：

- 最近最值钱的是两个 bugfix：
  - `3.19.9`
  - `3.19.15` 的 `d986d5d11`
- 最近最危险的是 `3.19.18+` 带来的协议和构建层漂移

因此，当前最稳妥的维护策略是：

- 短期内把高价值 bugfix 当成 backport 候选处理
- 不把整版 `3.19.x` 升级和局部 bugfix 吸收混成一次改动
- 等 FBS/IP​​C 和构建链准备好后，再考虑较大版本跨度的正式升级
