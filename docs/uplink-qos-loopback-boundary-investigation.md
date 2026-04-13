# 上行 QoS Loopback 边界排查纪要

日期：`2026-04-13`

> 这份文档记录本次围绕 `BW2` 的排查结论、已落地修正和后续建议。
> 它不替代最终结果文档，而是作为 loopback 边界 case 的专项调查说明。

## 1. 问题现象

`BW2` 在当前仓库里出现了这样的分化：

- synthetic runner 下通常保持 `stable/L0`
- browser loopback matrix 下会在 impairment phase 早期冲到 `congested/L4`
- recovery 期通常又能回到 `stable/L0`

当前 `BW2` 的网络切换不是“整段都是 `2000kbps`”，而是：

- baseline：`4000kbps / RTT 25ms / loss 0.1% / jitter 5ms`
- impairment：`2000kbps / RTT 25ms / loss 0.1% / jitter 5ms`
- recovery：`4000kbps / RTT 25ms / loss 0.1% / jitter 5ms`

因此，`BW2` 的本质是带宽边界转场 case，不是静态弱网 baseline case。

## 2. 本次排查得到的事实

### 2.1 `4000 -> 2000` 不是把真实发送速率从 `4Mbps` 砍到 `2Mbps`

loopback source 的真实发送目标大约在 `0.8 ~ 0.9Mbps`。

在 baseline `4000kbps` 阶段，trace 里常见值大约是：

- `sendBitrateBps ≈ 0.7 ~ 0.9Mbps`
- `targetBitrateBps ≈ 0.9Mbps`

因此，问题不是“`2000kbps` 物理上装不下 baseline 发送速率”。

### 2.2 真实触发项不是 case 参数本身，而是转场后的控制环路波动

切到 impairment 后，loopback matrix 经常出现：

- `qualityLimitationReason=bandwidth`
- `targetBitrate` 快速下调
- `sendBitrate` 在切档初期短时 overshoot
- `jitter` 明显升高
- 有时 RTT 也会短时升高

这会把状态机快速推进到：

- `stable -> early_warning`
- `early_warning -> congested`
- 然后继续降到 `L4`

### 2.3 之前直接读取 `remote-inbound-rtp` 的 RTT/jitter 会把问题放大

本次加了 targeted instrumentation 后确认：

- `remote-inbound-rtp.timestamp` 会连续 `2 ~ 3` 个采样周期不变
- 也就是 remote stats 不是每拍都新鲜
- 之前直接把 `remote-inbound-rtp.roundTripTime/jitter` 喂给快环，会把 stale sample 连续作用到多个采样周期

### 2.4 即使修掉 stale sample，`BW2` 在 real loopback 下仍然不是稳定 mild case

本次已落地两项观测修正后，`BW2` 仍可能在 loopback matrix 下冲到 `congested/L4`：

- RTT 快环优先改为 `candidate-pair.currentRoundTripTime`
- `remote-inbound-rtp.timestamp` 不推进时，不再把旧 RTT/jitter 当成新样本更新

这说明剩余问题已经不主要是“读错 stats”，而是：

- loopback runner 本身对 `4000 -> 2000` 这类边界转场会产生较强瞬态
- `BW2` 在 real loopback 下并不是稳定的 mild boundary case

### 2.5 最新 full matrix 说明主 gate 已收敛，boundary case 仍需分开处理

`2026-04-13T03:34:38.058Z` 的 full matrix artifact 已确认是全量主 gate：

- `selectedCaseIds = "all"`
- `includedCaseIds` 共 `43` 个
- `summary.total = 43`
- `summary.passed = 43`

这轮 full matrix 里：

- `BW2` 没有进入主 gate，因为它仍被标记为 `extended`
- `R4 / J3 / T9 / T1 / S4` 在当前主 gate 中都保持通过
- 主 gate 当前结论已经回到 `43 / 43 PASS`

但对比更早的 full matrix 快照，仍能看到 boundary evidence：

- `2026-04-12T08:12:22.359Z` 中，`T1 = stable/L0`，`S4 = early_warning/L1`
- `2026-04-12T17:03:58.344Z` 中，`T1 = congested/L4`，`S4 = congested/L2`
- `2026-04-13T03:34:38.058Z` 中，当前主 gate 已按最新 expectation 收回到 `43 / 43 PASS`

这说明：

- `T1 / S4` 的波动窗口是历史上真实观测到的 loopback boundary evidence
- 当前对它们追加最小 `expectByRunner.loopback` 后，默认主 gate 已重新收敛
- `BW6 / BW7` 那类“impairment 合格但 recovery 不过”的失败，则是评估逻辑 bug，不属于产品或 runner 新边界

## 3. 为什么 loopback 会把 mild case 打成 severe

当前 loopback matrix 有几个天然放大器：

1. `tc netem rate` 挂在 `lo` 根队列上
   它会同时影响 RTP、RTCP 和反馈包，不是只影响媒体 payload。

2. 同一浏览器进程内做 sender/receiver loopback
   媒体发送、反馈、解码和 stats 采样都共享同一进程调度。

3. QoS controller 在转场中会执行连续 `setEncodingParameters`
   切档本身会造成短时 target/send 不匹配。

4. matrix 对非 baseline case 以 impairment phase 的 `peak` 作为判定口径
   只要 early phase 冲到一次 `congested/L4`，后面即使恢复也仍会判 `过强`。

因此，`BW2` 的失败不应简单理解为“`2000kbps` 太小”，而应理解为：

`在当前 loopback runner 形态下，4000 -> 2000 的带宽切换会引发足够强的瞬态，使 QoS peak 落到 severe 区间。`

## 4. 本次已落地的修正

### 4.1 观测层修正

已落地：

- `candidate-pair.currentRoundTripTime` 优先于 `remote-inbound-rtp.roundTripTime`
- `remote-inbound-rtp.timestamp` 不推进时，不再把旧 RTT/jitter 当成新样本
- 当前 snapshot 缺少新鲜 transport metrics 时，不重复推进 RTT/jitter 的 EWMA

目的：

- 让快环控制更多依赖 transport RTT
- 避免 stale RTCP-derived stats 连续放大误判

### 4.2 runner 层修正

已落地：

- `loopback_runner.mjs` 中 `netem` 改为一次性下发完整 `qdisc replace`
- 不再先下发 delay/loss 再二次 replace 带 rate，减少 phase 切换时的附加尖峰

### 4.3 文档层修正

已落地：

- 逐 case 报告显式展示：
  - `baseline 网络`
  - `impairment 网络`
  - `recovery 网络`

这样像 `BW2` 这类转场 case 不再被误读成“整段都只有 impairment 那组参数”。

## 5. 当前建议口径

### 5.1 不把 `BW2` 的剩余问题定义成产品 QoS bug

当前更合理的归类是：

- 产品观测链路之前有可修正的问题，本次已修
- `BW2` 剩余失败主要来自 loopback runner 的边界瞬态

因此，它更接近：

- runner / measurement artifact
- loopback boundary case

而不是“产品 QoS 状态机必然错误”

### 5.2 `BW2` 不适合作为当前默认 loopback blocking gate

原因：

- synthetic 和 loopback 对 `BW2` 的行为已经证明不完全同构
- loopback 下它对转场瞬态过于敏感
- 当前 impairment `peak` 判定会放大这种敏感性

更合理的做法是：

- 将其作为 boundary / extended case 保留
- targeted 观察仍然继续
- 不再让它作为默认 blocking gate 的稳定判据

### 5.3 当前已落地的门禁策略

截至 `2026-04-13`，当前仓库采用下面的分层处理：

- `BW2`
  继续保留为 `extended` + targeted sentinel；
  保持 strict default expectation；
  不回退进默认主 gate。

- `T1`
  作为默认主 gate 中与 `BW2` 同构的转场 case；
  新增最小 `expectByRunner.loopback`，接受 loopback 下从 `stable` 到 `congested/L4` 的已观测波动窗口；
  仍要求 recovery 回到 baseline。

- `S4`
  作为 burst jitter 的 loopback 边界 case；
  新增最小 `expectByRunner.loopback`，接受 `early_warning/L1 ~ congested/L2` 的已观测窗口；
  仍要求 recovery 回到 baseline。

- `BW6 / BW7`
  当前不再作为 boundary case 处理；
  之前的失败来自 recovery 判定 bug，修正评估逻辑后恢复为 `PASS`。

## 6. 后续最佳方案

### 6.1 expectation 按 runner 维度拆分

目标：

- synthetic runner 和 loopback runner 不再强制共用一套 expectation

建议结构：

```json
"expect": { ... },
"expectByRunner": {
  "synthetic": { ... },
  "loopback": { ... }
}
```

当前仓库已补上基础解析能力，并已先落最小集：

- `T1.expectByRunner.loopback`
- `S4.expectByRunner.loopback`

当前暂未继续下沉为 runner-specific 的 case：

- `BW2`
  仍作为 strict targeted sentinel 保留，避免把边界问题“改 expectation 后看不见”。
- `R4 / J3 / T9`
  在 latest full matrix 主 gate 中均通过，当前没有新增 runner-specific expectation 的必要。

### 6.2 loopback 和真实链路的职责拆分

建议长期分工：

- `browser loopback + netem`
  主要验证状态机、动作链路、恢复逻辑是否工作
- `browser -> real SFU + netem`
  主要验证 boundary case 的真实口径

### 6.3 artifact 默认保留足够分析的数据

建议：

- 所有 case 默认保留三段网络参数
- 失败 case 自动附加结构化 signal summary：
  - `sendBitrate`
  - `targetBitrate`
  - `rtt`
  - `jitter`
  - `qualityLimitationReason`
- raw remote stats 只在 targeted / failure path 保存，不进常规 full report

## 7. 一句话结论

`BW2` 当前不过，不是因为“2000kbps 静态上装不下发送流”，而是因为在现有 real browser loopback runner 下，4000 -> 2000 的带宽转场会诱发足够强的反馈/队列/jitter 瞬态，导致 impairment phase peak 被打到 congested/L4；当前默认主 gate 已通过，但 `BW2` 继续作为扩展边界哨兵保留，runner 层边界特性仍需继续打磨。`
