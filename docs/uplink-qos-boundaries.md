# 上行 QoS 职责边界说明

日期：`2026-04-12`

> **文档性质**
>
> 这是内部技术边界说明，用来回答：
> 当前仓库里的 uplink QoS 到底做了什么，
> 哪些能力来自 `libwebrtc + mediasoup-worker`，
> 哪些能力是本仓库在其上补齐的策略控制层。

## 1. 一句话结论

`当前仓库的 uplink QoS 不是在重写 WebRTC 底层传输修复机制；底层 NACK / RTX / FEC / PLI / FIR / BWE 主要由 libwebrtc 和 mediasoup-worker 提供，本仓库补的是更上层的发送策略 QoS。`

## 2. 两层职责划分

| 层级 | 主要负责方 | 当前职责 |
|---|---|---|
| 媒体传输修复层 | `libwebrtc` + `mediasoup-worker` | 处理 RTCP feedback、重传、FEC/RED/RTX、关键帧请求、基础拥塞控制与 RTP/RTCP 行为 |
| 策略控制层 | 本仓库 client QoS + server QoS control plane | 基于 stats 做状态判断、编码梯度降级/恢复、`qosPolicy` / `qosOverride` / 自动 override / room pressure |

理解上可以把它拆成两句：

- 底层媒体链路尽量把“当前流”修回来。
- 策略层在判断“当前发送条件持续恶化”后，主动调整发送策略。

## 3. 底层自动提供的能力

下面这些能力，当前实现主要依赖浏览器 WebRTC 栈和 `mediasoup-worker`，不是这次 uplink QoS 新增的主线：

- `NACK`：接收端发现丢包后请求重传。
- `RTX`：通过重传 SSRC/codec carrying retransmission payload。
- `FEC / RED`：通过冗余或前向纠错提高抗丢包能力。
- `PLI / FIR`：接收端请求发送端尽快补关键帧。
- `Transport-CC / REMB`：基础带宽估计与拥塞反馈。
- RTP/RTCP 收发、重传、基础拥塞控制、关键帧协商等媒体层行为。

本仓库对这些反馈的关系主要是：

- 依赖底层媒体栈处理。
- 在 stats 侧做观测和透出，不自己实现独立的 `NACK/PLI/FEC/RTX` 响应策略。

一个直接证据是 [Producer.cpp](../src/Producer.cpp) 的 `getStats()` 会把底层统计暴露出来，例如：

- `nackCount`
- `nackPacketCount`
- `pliCount`
- `firCount`
- `roundTripTime`
- `jitter`

这说明当前仓库“知道这些反馈存在”，但并没有在 uplink QoS 状态机里单独对 `NACK/PLI/FIR` 做独立分支控制。

一个容易混淆的点是 `FEC`：

- 如果发送端是 Chrome 等浏览器，`FEC` 机制本身主要来自浏览器内置的 `libwebrtc`。
- 应用层能影响的更多是“能力声明 / codec 参数 / SDP 协商”，而不是在页面 JS 中逐秒直接操控底层 FEC 算法。
- 当前仓库只显式暴露了 `Opus` 音频 FEC 相关参数，并未把 `FEC` 做成 uplink QoS 状态机动作。

当前代码里可以看到：

- [Producer.d.ts](../src/client/lib/Producer.d.ts) 暴露了 `opusFec?: boolean`。
- [MediaSection.js](../src/client/lib/handlers/sdp/MediaSection.js) 会把 `opusFec` 映射为 SDP 参数 `useinbandfec=1/0`。
- [supportedRtpCapabilities.h](../src/supportedRtpCapabilities.h) 也声明了 `audio/opus` 的 `useinbandfec=1` 默认能力。

这说明当前仓库对 `Opus FEC` 的关系更接近：

- 暴露能力和协商参数。
- 允许上层在建链时表达偏好。
- 不在当前 QoS 动作层里把 `FEC` 当成高频动态调节杆。

当前 QoS 动作模型本身也能侧面说明这一点：

- [types.d.ts](../src/client/lib/qos/types.d.ts) 里的动作类型主要是 `setEncodingParameters`、`setMaxSpatialLayer`、`enterAudioOnly`、`pauseUpstream`、`resumeUpstream` 等。
- [planner.js](../src/client/lib/qos/planner.js) 和 [executor.js](../src/client/lib/qos/executor.js) 也没有单独的 `FEC` 动作分支。

## 4. 当前仓库新增或主线实现的 uplink QoS 能力

这次上行 QoS 的重点不是底层传输修复，而是更上层的发送策略控制。

### 4.1 client 侧策略能力

- 采样 `RTCPeerConnection.getStats()` / producer stats。
- 提取多维信号，而不只看带宽：
  `sendBitrateBps / targetBitrateBps / lossRate / rttMs / jitterMs / qualityLimitationReason / bandwidthLimited / cpuLimited`
- 运行 QoS 状态机：
  `stable -> early_warning -> congested -> recovering`
- 根据状态执行动作：
  `setEncodingParameters`、必要时 `audio-only`、`video pause`、恢复探测。

主要入口：

- [controller.js](../src/client/lib/qos/controller.js)
- [stateMachine.js](../src/client/lib/qos/stateMachine.js)

### 4.2 server 侧策略能力

- 接收并校验 `clientStats`。
- 将 client 侧 QoS snapshot 存入 registry 并做聚合。
- 对外提供 `qosPolicy` 与 `qosOverride` 控制链路。
- 生成 automatic override：
  `poor / lost / clear`
- 生成 room pressure override。
- 广播 connection quality / peer QoS / room QoS 相关通知。

主要入口：

- [RoomService.cpp](../src/RoomService.cpp)

## 5. 当前明确不覆盖或不主打的范围

下面这些不应被表述成“当前 uplink QoS 已完整覆盖”：

- 不替代 `libwebrtc` / `mediasoup-worker` 的底层拥塞控制实现。
- 不自研 RTP/RTCP 层的 `NACK / PLI / FIR` 响应逻辑。
- 不把 `FEC / RED / RTX` 做成当前主状态机的一等动态控制动作。
- 不把 `NACK/PLI/FIR` 计数本身作为当前主状态机的一等决策输入。
- 不宣称浏览器 Web SDK 已具备按秒级热切换 `NACK/FEC` 底层策略的控制权。
- 当前主线是 publisher uplink QoS，不是完整的 subscriber/downlink QoS。

因此，如果有人问“比如 `NACK`、`PLI` 的响应是不是这次新做的”，当前正确回答应是：

`不是。它们主要还是 WebRTC/mediasoup 底层自动提供的；这次仓库里新增和重点建设的是更上层的发送策略 QoS。`

## 6. 为什么“什么时候用 NACK，什么时候用 FEC”这个讨论仍然有意义

这个讨论本身是有意义的，但它主要属于“媒体引擎 / 原生 SDK / SFU”层，而不一定属于“浏览器页面 JS”层。

更准确地说，下面这类问题都是真实存在的工程问题：

- 在轻微随机丢包下，是优先依赖 `NACK/RTX` 还是增加冗余。
- 在高 RTT 场景下，重传时延是否已经高到需要更多依赖 `FEC/RED`。
- 在语音场景下，是否值得接受一定码率开销来换取更稳的可懂度。

但要注意控制权边界：

- 在浏览器 Web SDK 中，应用通常拿不到一个通用的、可按秒级热切换的底层 `FEC/NACK` 开关。
- 浏览器页面能控制的更多是：codec 参数、是否协商某类能力、是否触发重协商、以及码率/分辨率/帧率/层级控制。
- 真正决定某一帧、某一包是否带冗余、是否走哪类内部恢复机制的，往往还是 `libwebrtc` 和编码器自身策略。

因此，讨论本身并不空泛，只是要说清楚它发生在哪一层。

## 7. 浏览器 Web SDK、原生 SDK、SFU 的控制权差异

这部分建议作为统一口径：

- `浏览器 Web SDK`
  受浏览器暴露的 WebRTC API 限制，不能越过浏览器直接操控其内部 `libwebrtc` 私有 FEC/NACK 决策。
- `原生 SDK`
  如果自己集成 `libwebrtc`，可以暴露更深的编解码和传输参数，甚至实现自己的内部策略。
- `SFU / 媒体服务器`
  可以参与反馈、层选择、拥塞控制、是否宣告某些 codec/feedback 能力，但也不能完全替发送端编码器做所有逐包决定。

结合当前仓库，可以把边界概括成：

- 如果是浏览器发送端，本仓库更适合做“高层发送策略”控制。
- 如果未来有自研原生 SDK，则可以把 `FEC / RED / NACK / RTX` 的更多细粒度策略下沉到 SDK 内部实现。

## 8. LiveKit 可作为怎样的参照

如果用 LiveKit 作为行业参照，可以用下面这类说法：

- LiveKit 有客户端 SDK，但其 `Web SDK` 仍然受浏览器 WebRTC API 限制，并不会穿透浏览器去直接操控底层私有媒体栈。
- LiveKit 在客户端公开的常见媒体策略，更偏向于音频 `RED`、视频 `simulcast`、`dynacast`、`adaptiveStream` 这类较高层的可控项。
- 更底层的 `NACK / RTX / congestion control` 仍主要依赖 WebRTC 栈和 SFU 内部实现。

因此，如果有人问“为什么很多系统都会讨论 `NACK/FEC` 取舍，但业务层看起来又没有直接按钮”，比较准确的回答是：

`这类取舍是真实存在的，但通常落在媒体引擎层或 SFU 层；浏览器业务层常见的是能力协商和高层发送策略，而不是逐包控制。`

## 9. Simulcast 与 SVC 的区别

在讨论上行视频策略时，建议统一下面这组术语：

- `Simulcast`
  同一视频源同时编码并发送多路独立码流，例如 `180p / 360p / 720p`。
- `SVC`
  同一路码流内部做分层编码，接收侧或 SFU 可以只取基础层，或再叠加增强层。

两者的工程差异可以概括为：

- `Simulcast` 更成熟、更常见，SFU 选择哪一路转发也更直接，但编码成本通常更高。
- `SVC` 通常编码效率更好、层级切换更细，但依赖 codec 和端侧支持，工程约束也更多。
- 两者并不绝对对立，某些方案里也可能组合使用。

如果要用一句最短的话描述：

- `Simulcast` 是“多路独立备选版本”。
- `SVC` 是“一路里面切层”。

## 10. 内部建议口径

内部说明时，建议直接用下面这句：

`底层传输修复和基础拥塞控制，主要靠 libwebrtc + mediasoup-worker；当前仓库补齐的是面向业务控制的高层 uplink QoS 策略。`

如果要再展开一层，可以补一句：

`也就是说，底层负责尽量把流救回来，策略层负责在持续恶化时主动降级和恢复。`

如果讨论进一步延伸到 `FEC / RED / NACK / RTX`，建议继续补这一句：

`这些机制的取舍本身很重要，但在浏览器发送端里通常属于 libwebrtc 和协商层能力；当前仓库并没有把它们做成页面 QoS 状态机里的高频动态开关。`
