# 上行 QoS 下一阶段优先级路线图

日期：`2026-04-12`

## 1. 当前判断

当前 uplink QoS 已经具备：

- client publisher QoS 状态机 / ladder / probe
- server `clientStats` 聚合与 automatic override
- Node harness / browser harness / browser matrix
- 逐 case 结果文档与统一测试脚本

补充进展：

- `2026-04-12` 当前 `41` 个 browser matrix case 已全部 `PASS`
- matrix 判定、逐 case 结果文档和 targeted rerun 产物已经完成对齐
- matrix 已从“收敛目标”转为“长期稳定性 / 归档门禁”问题

按“uplink QoS 单能力线”评估，当前大致可给到 `7/10`。  
按“完整 RTC QoS 产品成熟度”评估，仍明显低于 LiveKit，主要差距在 subscriber/downlink QoS、长期生产磨损数据、跨端稳定性和运行时可观测性。

当前仍不建议把这套 QoS 当作“production-ready 且默认开启”的状态直接并入主线对外能力；但已经适合进入主干继续迭代。

## 2. 优先级总表

### P0：合入主干前必须完成

| 排名 | 事项 | 原因 | 目标结果 |
|---|---|---|---|
| 1 | `setQosOverride` 权限控制 | 当前任意 peer 仍可给其他 peer 下发 QoS override，属于安全问题 | 仅允许 self 或具备明确授权角色的调用者设置 override |
| 2 | `seq reset` 规则与边界收口 | 当前 refresh/reconnect 场景仍有 QoS 失效风险，且边界测试未锁死 | 明确 `>1000` 规则、补齐边界测试、确保刷新后 QoS 不失效 |

### P1：主线化后第一阶段重点

| 排名 | 事项 | 原因 | 目标结果 |
|---|---|---|---|
| 3 | full matrix 稳定性固化与归档 | 当前 `41 case` 已 PASS；full / targeted / archive 产物已拆分，但仍缺少连续多轮门禁 | 连续多轮可重复 PASS，并保留独立 full-matrix artifact / gate |
| 4 | override 语义彻底收口 | manual / automatic / room pressure / TTL clear 虽已可用，但语义仍偏实现驱动 | 文档、测试、实现三者完全一致 |
| 5 | 运行时 QoS 可观测性 | 现在测试侧可观测性比运行时强，线上排障仍偏弱 | 暴露 metrics / counters / current QoS state / override stats |

### P2：能力升级阶段

| 排名 | 事项 | 原因 | 目标结果 |
|---|---|---|---|
| 6 | subscriber/downlink QoS 设计与落地 | 这是与 LiveKit 最大的能力差距 | 形成双向 QoS，而不是只有 uplink |
| 7 | 浏览器 / 终端覆盖扩展 | 当前主要依赖 host + headless browser + loopback | 覆盖更多浏览器和更真实通话拓扑 |
| 8 | policy / profile 系统化 | 现在 profile 与策略仍偏代码内嵌 | 支持更清晰的配置、版本化和策略演进 |

### P3：工程收尾与长期维护

| 排名 | 事项 | 原因 | 目标结果 |
|---|---|---|---|
| 9 | 测试与文档单一事实源继续收口 | 现在已经比之前好很多，但仍需长期维护 | 跑脚本即可产出最终报告，无需人工改多份文档 |
| 10 | 算法细调（降级节奏、probe 敏感度） | 这是体验优化，但优先级低于安全和双向能力补齐 | 在不引入新回归的前提下优化体验 |

## 3. 逐项说明

### 1. `setQosOverride` 权限控制

这是唯一明确的 P0 安全问题。  
如果不补，这套 QoS 即使测试再全，也不适合作为默认暴露能力进入 `main`。

建议产出：

- `SignalingServer -> RoomService::setQosOverride()` 传递调用者身份
- 明确授权规则
- 集成测试覆盖“越权设置失败”

### 2. `seq reset` 收口

这是唯一明确的 P1 功能阻塞问题。  
当前实现已经开始支持 reset，但边界和测试还没完全锁死。

建议产出：

- 明确 `seq` 回退阈值语义
- 增加 `1000 / 1001` 边界测试
- 增加 refresh/reconnect 后 QoS 仍可继续上报的集成测试

### 3. full matrix 稳定性固化与归档

这是“从当前一次收敛”走向“可长期维持”的核心标志。
当前主链路已经实现 `41 case` 全绿，但如果没有连续多轮复现和独立归档 artifact，后续回归风险仍不够透明。

建议产出：

- 全量 `41 case` 连续多轮可复现 `PASS`
- full matrix / targeted rerun / archive 快照继续保持隔离，避免后续回归再次串档
- 可以作为 nightly / pre-merge gate 长期运行

### 4. override 语义收口

当前 merge / clear / manual clear 已经有实现和回归测试，但后续继续演进前，最好先把语义写死。

建议产出：

- manual / automatic / room pressure / TTL clear 的优先级表
- clear 作用域说明
- 文档和测试都以此为准

### 5. 运行时可观测性

现在测试报告很详细，但线上运行时仍缺核心指标。

建议产出：

- poor / lost / stale peer 数量
- automatic override 触发次数
- rejected clientStats 数量
- 当前 peer QoS 级别 / ladderLevel 分布

### 6. subscriber/downlink QoS

这是与 LiveKit 最大的能力差距。  
如果下一阶段还只是继续调 uplink 阈值，收益会明显递减。

建议产出：

- 订阅侧层选择
- 优先级与带宽分配
- dynacast / forwarding 层联动

## 4. 合入 `main` 的建议口径

### 可以合入主干继续迭代的前提

- 明确标识为 QoS ongoing / experimental
- 当前 `41 case` full matrix 已全绿，且结果透明可见
- 不把这套 QoS 宣称为 production-ready 默认能力

### 合入前必须补齐的最低条件

- `setQosOverride` 权限控制
- `seq reset` 收口并补测试

### 合入后第一阶段的验收目标

- `41 case` full matrix 连续多轮稳定 PASS，并保留独立归档 artifact
- override 语义完全稳定
- 运行时可观测性补齐

## 5. 最短行动清单

如果只做最重要的 5 件事，建议按下面顺序推进：

1. `setQosOverride` 权限控制
2. `seq reset` 规则与边界测试
3. 把 `41 case` full matrix 固化为可重复、可归档的长期 gate
4. override 语义文档化并锁定
5. 启动 subscriber/downlink QoS 设计
