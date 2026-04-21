# QoS 测试覆盖总览

本文不是“最新一次测试结果快照”，而是当前仓库里 QoS 自动化测试的**规格总览**。

重点回答四个问题：

1. 上下行 QoS 现在分别有哪些测试入口
2. 每个场景 case 的测试目的是什么
3. 每个场景 case 是怎么测的
4. 每个场景 case 通过/失败时应该怎么分析

说明：

- 这里的“case”优先指**场景驱动的 harness / matrix case**。
- `client-js` / `cpp-unit` / `cpp-integration` 这类底层测试粒度较细，本文按**能力簇**总结，不逐条抄所有断言。
- 最新一次运行通过/失败状态，仍以 case report / generated JSON / CI artifact 为准。

## 1. 测试分层

当前 QoS 自动化大致分成四层：

| 层级 | 作用 | 典型入口 |
|---|---|---|
| client JS / Node 逻辑层 | 验证纯逻辑、状态机、协议和控制器行为 | `src/client/lib/test/test.qos.*.js` |
| C++ 服务端层 | 验证 ingest / validate / aggregate / override / allocator / cleanup | `mediasoup_qos_unit_tests` / `mediasoup_qos_integration_tests` |
| browser harness 层 | 验证真实浏览器中的 signaling / WebRTC / consumer-producer 控制链路 | `tests/qos_harness/browser_*.mjs` |
| matrix 层 | 验证弱网矩阵、恢复路径、边界场景和趋势分析 | `run_matrix.mjs` / `run_downlink_matrix.mjs` |

## 2. Uplink QoS

### 2.1 client-js 能力簇

主要覆盖 `src/client/lib/test/test.qos.*.js`：

- controller
- coordinator
- planner
- executor
- protocol
- signalChannel
- stateMachine
- sampler
- statsProvider
- factory
- peerSession
- signals / probe

测试目的：

- 验证 uplink QoS 状态机迁移是否正确
- 验证 planner 是否根据 snapshot / policy / override 生成正确动作
- 验证 executor / signalChannel / protocol 的接口语义是否一致
- 验证 controller 在固定输入下是否给出稳定决策

测试方法：

- 不启动 SFU
- 使用 Jest + fake stats provider / fake action executor / fake signal channel
- 直接注入 snapshot、policy、override 和时钟推进

结果分析口径：

- 如果这层失败，优先判断为**client 纯逻辑问题**
- 这层通过，不代表浏览器真实链路一定没问题；只能说明 client library 的纯逻辑没有跑偏

### 2.2 C++ 服务端能力簇

#### `cpp-unit`

覆盖内容：

- client QoS controller / planner / protocol / state machine
- QoS protocol / validator
- QoS registry / aggregator / room aggregator
- override builder
- downlink allocator / health / planner / demand / publisher supply
- threaded control helper / QoS thread model

测试目的：

- 保证服务端基础数据结构和纯算法层正确

测试方法：

- 直接构造协议 payload、聚合输入和 override 期望
- 不依赖真实浏览器

结果分析口径：

- 失败通常归因于**协议、校验、聚合算法或 builder 逻辑**
- 如果 unit 已经失败，不需要先看 browser harness

#### `cpp-integration`

uplink 相关能力覆盖：

- `clientStats` snapshot 存储与聚合
- QoS 聚合结果进入广播
- malformed payload 拒绝
- 旧 `seq` 忽略
- join 时下发 QoS policy
- 手动 override 下发
- 手动 clear 生效
- 自动 poor / lost override
- 质量恢复后 automatic clear
- connection quality 通知
- room state / room pressure override

测试目的：

- 验证 uplink QoS 的服务端主链路确实闭环

测试方法：

- 启动真实 SFU
- 用测试客户端直接发 websocket 请求和 snapshot
- 从服务端读回 stats / notification / override

结果分析口径：

- 失败通常归因于**服务端 ingest / aggregate / notification / cleanup 链路**
- 如果 client-js 通过、cpp-integration 失败，通常优先看 server 侧

### 2.3 Node harness 场景

这组场景由 [run.mjs](../../../tests/qos_harness/run.mjs) 驱动。

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| `publish_snapshot` | 验证正常 client snapshot 可被服务端接收、保存并聚合成 QoS 状态 | 启动 SFU；用 Node 客户端 join；本地 QoS controller 连续采样并 publish 两次 snapshot；最后读取 `getStats` | 失败通常说明 `clientStats` publish、seq 递增、聚合或 stats 回读有问题 |
| `stale_seq` | 验证旧 snapshot 不会覆盖更新 snapshot，且响应能显式反映 ignored/store 结果 | 先发 `seq=50`，再发 `seq=49`；检查第二次响应里的 `stored=false/reason=stale-seq`，再读取服务端 stats 检查最终 seq 和 quality | 如果旧 seq 覆盖了新状态，或响应不再暴露 ignored 原因，说明 stale-seq 防护或可观测性回退了 |
| `policy_update` | 验证 policy 更新能送达并改变 runtime 配置 | join 后构造 controller；发 `setQosPolicy`；读取 runtime policy / controller 行为 | 失败通常是 policy 分发、解析或 runtime 应用链路断了 |
| `auto_override_poor` | 验证差网络 snapshot 会触发 automatic override | 发 `peerState.quality=poor`、高 RTT / 丢包 snapshot，等待 `qosOverride` | 没收到 override：优先看 aggregate / automatic override 生成；收到错误 reason：看 poor/lost 判定 |
| `override_force_audio_only` | 验证手动 `forceAudioOnly` override 能驱动 client 执行动作 | 发 `setQosOverride(forceAudioOnly=true)`，controller 再采样一次，检查执行动作 | 失败说明 override 通知没送到，或 controller 没把 override 落成动作 |
| `manual_clear` | 验证手动 override 用 `ttlMs=0` clear 后，client 回到本地 QoS 控制 | 先发 manual override，再发 clear override，检查 controller 后续行为恢复本地决策 | 失败说明 clear 语义、ttl=0 处理或 controller override merge 有问题 |

### 2.4 browser harness 场景

#### `browser_server_signal.mjs`

| 子场景 | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| policy apply | 验证浏览器端收到 policy 后 runtime 配置立即更新 | 真实 headless Chromium join；调用 `setPolicy`；读取 runtime policy | 失败说明 browser-side signal channel 或 policy 应用链路有问题 |
| auto override trigger | 验证浏览器端发差网络 snapshot 后能收到 automatic override | 在浏览器里 publish 差网络 snapshot，等待 `server_auto_poor/server_auto_lost/server_room_pressure` | 没收到说明 server 自动 override 主链路断；收到错误 reason 说明判定口径漂移 |
| clear override trigger | 验证恢复 snapshot 后 automatic clear 能回到浏览器端 | 再发健康 raw snapshot，等待 `server_auto_clear/server_room_pressure_clear` | 没 clear 说明恢复检测或 clear 派发有问题 |

#### `browser_loopback.mjs`

| 子场景 | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| degrade under netem | 验证真实浏览器 loopback + UDP netem 下，uplink QoS 会感知到退化 | 启动浏览器 loopback；对 `lo` 注入 delay/loss；采集 trace 和 state | 如果没有明显退化，优先看 browser stats、sampler 或 planner 是否没吃到弱网条件 |
| recover after netem removal | 验证移除 netem 后状态能恢复到 baseline 水平 | 清掉 netem，继续采样，比较 `bestRecovered` 和 baseline | 如果恢复慢或不恢复，优先看 recovery path / probe / fast-path 逻辑 |

### 2.5 uplink browser matrix

这组 case 由 [sweep_cases.json](../../../tests/qos_harness/scenarios/sweep_cases.json) 定义，由 `run_matrix.mjs` 驱动。

统一测试方法：

- browser loopback synthetic/hybrid 弱网场景
- 每个 case 都跑 `baseline -> impairment -> recovery`
- 主要分析三个量：
  - impairment 期间的 `peak` 状态
  - recovery 期间的 `best` 状态
  - timing / trace 中的状态迁移和 level 变化

统一结果分析口径：

- `state` 看 QoS 状态强弱：`stable < early_warning < recovering < congested`
- `level` 越大表示降级越重
- `maxLevel` / `minLevel` 是允许的判定区间，不是唯一固定值
- transition / burst case 额外关注 recovery 是否回到 baseline 附近

#### Baseline

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| `B1` | 验证中等基线网络下保持 `stable/early_warning`，不应过度降级 | baseline 参数：`4000 kbps / 25 ms / 0.1% loss / 5 ms jitter` | 如果直接进入 `congested` 或高 level，说明默认阈值过敏 |
| `B2` | 验证较高质量网络下保持稳定，不应触发任何降级 | `8000 kbps / 20 ms / 0.1% / 3 ms` | 若出现非 `stable` 或 level>0，说明健康基线过强误判 |
| `B3` | 验证较差但仍可用的基线不会被误判为极端严重 | `2000 kbps / 55 ms / 0.5% / 12 ms` | 允许 `early_warning/congested` 区间，但若完全不动则说明灵敏度不足 |
| `B4` | 扩展：高带宽校准基线，验证高质量基线的稳定零动作 | `15000 kbps / 15 ms / 0.1% / 1 ms` | 主要是标定上界；默认不进 blocking gate |
| `B5` | 扩展：超高带宽校准基线，验证极佳条件下不误触发 | `30000 kbps / 10 ms / 0.1% / 1 ms` | 主要是高端基线哨兵；默认不进 blocking gate |

#### Bandwidth Sweep

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| `BW1` | 验证 3000 kbps 轻度受限时最多轻微退化 | impairment 带宽降到 `3000 kbps` | 若直接重度 `congested/L4`，说明带宽阈值过激 |
| `BW2` | 扩展：2000 kbps 边界哨兵，观察 loopback boundary 漂移 | impairment 带宽 `2000 kbps` | 这是边界 case，重点看稳定性，不作为默认 blocking gate |
| `BW3` | 验证 1000 kbps 时应明显进入重度拥塞 | impairment 带宽 `1000 kbps` | 如果仍然接近 stable，说明带宽敏感度不足 |
| `BW4` | 验证 800 kbps 下持续重度退化 | impairment 带宽 `800 kbps` | 如果不进入 `congested`，说明降级力度偏弱 |
| `BW5` | 验证 500 kbps 下强降级行为 | impairment 带宽 `500 kbps` | 若 level 不够高，说明 severe bandwidth case 未打满 |
| `BW6` | 验证 300 kbps 下接近极限拥塞 | impairment 带宽 `300 kbps` | 若仍可保持较低 level，说明极弱带宽模拟未真正生效 |
| `BW7` | 验证 200 kbps 下最重带宽拥塞 | impairment 带宽 `200 kbps` | 这是带宽最严苛 case，若没到重度拥塞要优先看 runner/network injection |

#### Loss Sweep

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| `L1` | 验证 0.5% loss 基本无感或仅轻微影响 | impairment 丢包 `0.5%` | 若过度降级说明 loss 阈值过敏 |
| `L2` | 验证 1% loss 可能触发轻微预警 | impairment 丢包 `1%` | 如果仍完全 stable 也未必错误，但不能过度拥塞 |
| `L3` | 验证 2% loss 进入轻微压力区间 | impairment 丢包 `2%` | 应至少接近 `early_warning` 语义 |
| `L4` | 验证 5% loss 进入明显退化区间 | impairment 丢包 `5%` | 若不出现 `early_warning/congested`，说明 loss 响应过弱 |
| `L5` | 验证 10% loss 时应进入严重拥塞 | impairment 丢包 `10%` | 若仍停在轻微退化，说明严重丢包感知不足 |
| `L6` | 验证 20% loss 时维持高等级降级 | impairment 丢包 `20%` | 重度 loss 仍不拥塞，说明判定失真 |
| `L7` | 验证 40% loss 极端场景 | impairment 丢包 `40%` | 主要看 severe case 是否被充分识别 |
| `L8` | 验证 60% loss 极端上界 | impairment 丢包 `60%` | 如果 runner 自身崩掉，优先判断为环境/浏览器稳定性问题 |

#### RTT Sweep

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| `R1` | 验证 `50 ms` RTT 仍应接近稳定 | impairment RTT `50 ms` | 若直接拥塞，说明 RTT 阈值过敏 |
| `R2` | 验证 `80 ms` RTT 可能进入轻微预警 | impairment RTT `80 ms` | 应允许 `stable/early_warning` |
| `R3` | 验证 `120 ms` RTT 轻中度压力 | impairment RTT `120 ms` | 若毫无变化，说明 RTT 响应太迟钝 |
| `R4` | 验证 `180 ms` RTT 进入明显退化，但不应过强 | impairment RTT `180 ms` | 这是历史上容易“判得过强”的 case，若直接 `congested/L4` 要重点审查 |
| `R5` | 验证 `250 ms` RTT 进入重度拥塞 | impairment RTT `250 ms` | 若仍较轻，说明高 RTT case 响应不足 |
| `R6` | 验证 `350 ms` RTT 极端高延迟 | impairment RTT `350 ms` | 属于 severe RTT 上界哨兵 |

#### Jitter Sweep

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| `J1` | 验证 `10 ms` jitter 基本稳定 | impairment jitter `10 ms` | 若明显退化，说明 jitter 基线太敏感 |
| `J2` | 验证 `20 ms` jitter 可能轻微预警 | impairment jitter `20 ms` | 应允许 `stable/early_warning` |
| `J3` | 验证 `40 ms` jitter 进入中度压力区间 | impairment jitter `40 ms` | 若仍完全 stable，说明 jitter 感知偏弱 |
| `J4` | 验证 `60 ms` jitter 进入明显退化区间 | impairment jitter `60 ms` | 若不出现 `early_warning/congested`，说明 jitter 判定偏迟钝 |
| `J5` | 验证 `100 ms` jitter severe case | impairment jitter `100 ms` | 若不能进入重度退化，说明 jitter severe path 未覆盖到位 |

#### Transition

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| `T1` | 验证 `4000 -> 2000 -> 4000` 的轻中度带宽转场 | baseline 正常、impairment 降带宽、recovery 回基线 | 如果恢复不到 baseline 附近，说明 recovery path 有问题 |
| `T2` | 验证 `4000 -> 1000 -> 4000` 的明显带宽转场 | 带宽中度恶化再恢复 | 重点看是否进入 severe，然后能否回来 |
| `T3` | 验证 `4000 -> 500 -> 4000` 的更重带宽转场 | 带宽 severe 恶化再恢复 | recovery 若太慢，要看 probe / fast-path |
| `T4` | 验证丢包从 `0.1% -> 5% -> 0.1%` 的转场 | 主要观察 loss 导致的状态切换与恢复 | 如果 impairment 不明显，说明 loss 感知不足；恢复慢则看恢复策略 |
| `T5` | 验证丢包从 `0.1% -> 20% -> 0.1%` 的 severe 转场 | severe loss 再恢复 | 重点看 severe -> recovery 是否完整闭环 |
| `T6` | 验证 RTT 从 `25 ms -> 180 ms -> 25 ms` 的转场 | RTT 压力再恢复 | 若 recovery 尾部拖长，通常看 RTT 恢复路径 |
| `T7` | 验证 jitter 从 `5 ms -> 40 ms -> 5 ms` 的转场 | jitter 压力再恢复 | 重点看 jitter 对恢复启动和稳定收尾的影响 |
| `T8` | 验证综合恶化场景下可重度退化，且允许 recovery 失败 | 综合 `bw/rtt/loss/jitter` 压力 | 这是明确允许 `recovery=false` 的 case，失败不等同实现错误，重点看是否有可解释的 severe 退化 |
| `T9` | blind-spot recovery：高质量基线突入长时 severe impairment | baseline 很高、impairment 极差且持续 100s | 重点不是“会不会退化”，而是 long-blind-spot 后 recovery 能否重新启动 |
| `T10` | blind-spot recovery：更高基线下的长时 severe impairment | 基线更高，长时 severe | 主要看 recovery tail oscillation 风险 |
| `T11` | blind-spot recovery：超高基线下的长时 severe impairment | 基线更高，长时 severe | 和 `T10` 类似，但更接近上限边界 |

#### Burst

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| `S1` | 验证短时高丢包 burst 会触发明显退化 | burst 丢包 `10%`，仅持续 `5000 ms` | 重点看短 burst 能否被检测，而不是长期稳态 |
| `S2` | 验证短时低带宽 burst 会触发严重退化 | burst 带宽 `300 kbps` | 如果 burst 期间没明显反应，说明短时冲击捕捉不足 |
| `S3` | 验证短时高 RTT burst 会触发轻中度退化 | burst RTT `200 ms` | 如果过重，说明 burst RTT 过敏；如果无感，说明短 burst 感知不足 |
| `S4` | 验证短时高 jitter burst 的边界响应 | burst jitter `60 ms` | 这是历史上存在 loopback 漂移的边界 case，重点看 runner-specific expectation 是否稳定 |

## 3. Downlink QoS

### 3.1 client / C++ 能力簇

#### `cpp-unit`

覆盖内容：

- allocator
- health monitor
- protocol / validator
- registry
- aggregator
- room aggregator
- override builder

测试目的：

- 保证 downlink 基础决策模型、协议约束和状态机推进正确

测试方法：

- 直接构造 subscription、snapshot、allocator 输入和 publisher supply 状态
- 不依赖真实浏览器

结果分析口径：

- 失败通常说明**纯服务端算法或协议层问题**

#### `cpp-integration`

downlink 相关能力覆盖：

- `downlinkClientStats` 存储
- malformed payload 拒绝
- legacy schema 接受
- stale `seq` 拒绝
- hidden 自动 pause consumer
- visible 自动 resume consumer
- large / small tile 获得不同 layer
- leave / reconnect 后 downlink 状态清理
- sustained zero-demand -> `pauseUpstream`
- demand restored after pause -> `resumeUpstream`

测试目的：

- 验证 downlink ingest / consumer control / publisher supply 的服务端主链路

测试方法：

- 启动真实 SFU
- 用测试客户端发 downlink snapshot / control request
- 读取 consumer state / override / cleanup 结果

结果分析口径：

- 失败通常优先判断为**服务端 downlink 主链路问题**

### 3.2 downlink browser harness 场景

#### `browser_downlink_controls.mjs`

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| case 1 `pauseConsumer` | 验证显式 `pauseConsumer` 控制链路可达 | 浏览器 join + consume 后，直接调用 server API `pauseConsumer` 并回读 consumer state | 若 `paused=true` 没设置，优先看 server control API 或 consumer state 回读 |
| case 2 `resumeConsumer` | 验证显式 `resumeConsumer` 控制链路可达 | 在 pause 后调用 `resumeConsumer` 并回读 state | 若 `paused=false` 没恢复，优先看 resume 控制链路 |
| case 3 `requestConsumerKeyFrame` | 验证 resume 后可请求关键帧 | 调用 `requestConsumerKeyFrame`，检查请求成功返回 | 失败通常说明 worker control/action 链路有问题 |

#### `browser_downlink_e2e.mjs`

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| case 1 `visible=false -> pause` | 验证 `downlinkClientStats` hidden 提示能驱动 server pause consumer | 发送 `visible=false` snapshot，稍等后读取 consumer state | 若未 pause，优先看 `downlinkClientStats` ingest / planner / applyActions |
| case 2 `visible=true -> resume` | 验证 `downlinkClientStats` visible 提示能恢复 consumer | 再发 `visible=true` snapshot，读取 state | 若未恢复，优先看 planner / controller 的 visible path |
| case 3a `hidden priority` | 验证 hidden 时 priority 进入最低档 | 发 hidden snapshot，读取 `priority` | 若优先级不变，说明 priority 映射或 apply path 有问题 |
| case 3b `visible grid priority` | 验证 visible grid 时 priority=120 | 发 visible grid snapshot，读取 `priority` | 若不是 120，说明 grid priority 配置或动作应用有问题 |

#### `browser_downlink_priority.mjs`

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| `PriorityCompetitionPinnedVsGrid` | 验证 pinned tile 在受限带宽下优于 grid | 单 subscriber、双 consumer，真实 browser getStats + netem，比较高优先级与低优先级 score | 若高优先级长期不优于低优先级，优先看 priority 或 budget allocator |
| `PriorityCompetitionScreenShareVsGrid` | 验证 screen-share 优先级高于 grid | 与上类似，但高优先级侧改为 screen-share 提示 | 若 screenshare 不能维持优势，说明优先级语义不够强 |
| `RecoveryAfterThrottleRelease` | 验证释放 throttle 后高优先级 consumer 恢复优势 | 先 throttle 再释放，持续采样 | 若恢复后高优先级不能重新领先，优先看 recovery / reallocation |
| `NoRegressionWithoutThrottle` | 验证无弱网时不会出现无意义 pause 或逆序 | 不注入 throttle，仅持续采样 | 若无弱网也出现 pause 或 priority 倒挂，说明基础逻辑回归 |

#### `browser_downlink_v2.mjs`

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| case 1 room-demand clamp | 验证低 demand 会给 publisher 下发 track clamp | 发低带宽/小 tile snapshot，等待 `downlink_v2_room_demand` | 若没 clamp，优先看 producer demand 聚合或 publisher supply builder |
| case 2 demand-restored clear | 验证高 demand 恢复后会 clear clamp | 再发高带宽/pinned 大 tile snapshot，等待 `downlink_v2_demand_restored` | 若 clamp 不 clear，说明恢复 path 有问题 |
| case 3 zero-demand hold | 验证 hidden 进入 v2 zero-demand hold clamp | 发 hidden snapshot，等待 `downlink_v2_zero_demand_hold` | 若没 hold，说明 zero-demand hold 状态机有问题 |

#### `browser_downlink_v3.mjs`

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| case 1 sustained hidden -> pauseUpstream | 验证持续 zero-demand 会真正 `pauseUpstream`，而不是只低层 hold | 连续 hidden snapshot 超过 `kPauseConfirmMs`，等待 active `pauseUpstream` override | 若没有 active pause，优先看 zero-demand state machine / supply controller |
| case 2 demand restored -> resumeUpstream | 验证 demand 恢复后会显式 `resumeUpstream` | hidden 触发 pause 后，再发 visible pinned snapshot，等待 active `resumeUpstream` | 若没有 resume，优先看 pause->warmup->resume 路径 |
| case 3 rapid flicker no pause | 验证短时 hide/show 抖动不会误触发 `pauseUpstream` | 快速交替发 visible/hidden，再检查该时间窗内是否出现新 pause | 若 flicker 仍触发 pause，说明防抖和确认窗口失效 |

### 3.3 downlink matrix

这组 case 由 [downlink_cases.json](../../../tests/qos_harness/scenarios/downlink_cases.json) 定义，由 `run_downlink_matrix.mjs` 驱动。

统一测试方法：

- 单 case 跑 `baseline -> impairment -> recovery`
- 每 500ms 左右注入一轮 downlink snapshot
- 记录：
  - phase 结束时 consumer state
  - override 序列
  - timing 指标
  - D7 竞争结果
  - D8 pause/resume 序列与振荡信息

统一结果分析口径：

- `preferredSpatialLayer` 越低说明接收层级越低
- `consumerPaused=true` 说明 subscriber receive control 已生效
- `pauseUpstream/resumeUpstream` 说明 producer-side zero-demand 协调已生效
- `recovers=true` case 重点看 recovery 结束时是否回到 baseline 附近

| Case | 测试目的 | 测试方法 | 结果分析口径 |
|---|---|---|---|
| `D1` | 验证好网络下所有 consumer 保持高层，不误 pause | baseline/impairment/recovery 都是 `5000 kbps / 30 ms / 0 loss / 2 ms jitter` | 若出现 pause 或层级明显下降，说明健康基线被误伤 |
| `D2` | 验证带宽降到 `300 kbps` 时会降层，恢复后回到高层 | impairment 带宽降到 `300 kbps` | 若 impairment 不降层，说明带宽感知不足；若 recovery 回不来，说明恢复 path 有问题 |
| `D3` | 验证 `15%` 丢包时 health degrade 和降层 | impairment 丢包 `15%` | 若不降层，说明 loss path 过弱；若过度 pause，要看阈值是否过激 |
| `D4` | 验证 `RTT 500 ms` 时 layer reduction | impairment RTT `500 ms` | 若无变化，RTT 敏感度不足；若直接 pause，说明 RTT case 判得过重 |
| `D5` | 验证 `jitter 80 ms` 时 layer reduction | impairment jitter `80 ms` | 主要看 jitter path 是否能稳定触发降层 |
| `D6` | 验证 `good -> bad -> good` 综合转场能恢复且不明显振荡 | impairment 使用 `200 kbps / 200 ms / 10 loss / 40 jitter` | 若 recovery 结束仍低层或 pause，说明恢复闭环不稳；若 override 序列来回跳，说明振荡控制不足 |
| `D7` | 验证单 subscriber 双 consumer 竞争时，pinned 大 tile 获得更好层级 | baseline/impairment/recovery 中都在同一 subscriber snapshot 内比较两个 consumer | 若高优先级 consumer 不优于低优先级，优先看 budget allocator / priority 语义 |
| `D8` | 验证所有 consumer hidden 时持续 zero-demand 会 `pauseUpstream`，恢复后 `resumeUpstream`，且序列不乱 | impairment 全 hidden，recovery 改回 visible pinned | 若 hidden 阶段没有 pause，说明 zero-demand pause 失效；若 recovery 没 resume，说明 warmup/resume 失效；若序列出现 `pause->resume->pause` 等回摆，说明有振荡问题 |

## 4. 当前未覆盖或只部分覆盖的点

虽然上下行 QoS 主链路已经有较完整覆盖，但下面这些点还不是“完全自动化签收”：

- 多浏览器覆盖
  当前 browser harness 主要基于 Chromium headless。
- 更接近真实公网链路的独立证据层
  现有大量 browser matrix 仍以 loopback / netem 为主。
- downlink 媒体恢复时间
  当前 `pauseUpstream` / `resumeUpstream` 重点覆盖控制面，不直接量化“resume 后首帧恢复时间”。
- `dynacast`
  当前 downlink 仍不是完整的 room-level adaptive stream + dynacast 闭环。
- room-level global bitrate budgeting
  当前 downlink 主要还是 subscriber receive control + zero-demand publisher coordination。

## 5. 常用入口

统一入口：

```bash
./scripts/run_qos_tests.sh
```

常见定向入口：

```bash
./scripts/run_qos_tests.sh client-js cpp-unit cpp-integration
node tests/qos_harness/browser_downlink_v3.mjs
node tests/qos_harness/run_downlink_matrix.mjs --cases=D7,D8
node tests/qos_harness/run_matrix.mjs --cases=T9,T10,T11
```

## 6. 相关阅读

- [uplink-qos-briefing.md](../uplink/briefing.md)
- [uplink-qos-final-report.md](../uplink/README.md)
- [uplink-qos-test-results-summary.md](../uplink/test-results-summary.md)
- [downlink-qos-test-results-summary.md](../../downlink-qos-test-results-summary.md)
- [downlink-qos-case-results.md](../../downlink-qos-case-results.md)
- [downlink-qos-v3-worker-validation_cn.md](../downlink/v3-worker-validation_cn.md)
- [README.md](../../../README.md)
