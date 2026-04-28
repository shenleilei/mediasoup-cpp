# Design

## Context

当前 QoS matrix 存在一个根本性问题：一个 case 的名义参数并不等于实际测试语义。

典型现象：

- 物理链路层：
  - `bandwidth` 会被 runner 再折算为更低的 netem rate
  - `rtt` 会被拆成单向 delay
- synthetic 判定层：
  - `rtt` 会被再次放大为 `reportedRttMs`
  - `qualityLimitationReason` 会被派生或特判
  - `sendCeilingBps` 会由 utilization 模型推导
- runner 层还可能有 case-specific 特判

这让 matrix 同时承担了两种相互冲突的职责：

1. 现实网络端到端验证
2. 控制器回归稳定性验证

设计目标不是简单“删掉某个放大规则”，而是把这两种职责彻底拆开。

## Proposed Testing Model

### 1. Physical E2E Matrix

#### Purpose

回答：

- 在现实物理网络条件下，browser / cpp QoS 实际表现如何？
- 某个 case 大致对应什么线上网络场景？

#### Rules

- case 参数字面即物理网络值
- `tc netem` 直接按 case 参数施加
- 不允许额外 RTT 放大
- 不允许 runner 侧带宽折算
- 不允许 case-specific synthetic 特判
- 允许记录真实浏览器 / 客户端观测到的 stats
- 允许 report 里展示“原始 case 值”和“真实观测 stats”，但不允许中间再造一个隐式“更真实”的输入值

#### Allowed Inputs

- `bandwidth`
- `rtt`
- `loss`
- `jitter`

#### Allowed Outputs

- 真实 trace
- 真实 state transition
- 真实 recovery timing
- 真实 transport / sender metrics

### 2. Synthetic Controller Matrix

#### Purpose

回答：

- 在确定性控制器输入下，状态机 / planner / executor 是否按预期工作？
- 某个 case 的 oracle 是否稳定？

#### Rules

- 不再伪装成现实网络测试
- case 定义中的输入字段直接描述“控制器输入”，而不是物理网络
- 允许：
  - synthetic RTT
  - synthetic loss
  - synthetic send ceiling
  - synthetic quality limitation reason
- 这些值必须显式写在 case 或报告中
- 不允许再借用物理网络字段名造成误导

#### Allowed Inputs

- `reportedRttMs`
- `lossRate`
- `sendCeilingBps`
- `jitterMs`
- `qualityLimitationReason`

#### Allowed Outputs

- deterministic verdict
- deterministic action count
- deterministic state transition timing

## Runner Realignment

### Browser Side

#### Current

- `run_matrix.mjs` 混合物理 netem、真实浏览器 stats、基础设施 heuristic

#### Target

- `run_matrix.mjs` 演进为 `browser physical-e2e matrix`
- 若仍需要 deterministic synthetic browser regression，则新增独立入口，例如：
  - `run_browser_synthetic_matrix.mjs`

### CPP Side

#### Current

- `run_cpp_client_matrix.mjs` 同时使用物理 netem 和 `QOS_TEST_MATRIX_PROFILE` synthetic 注入

#### Target

- 拆成两条：
  - `run_cpp_client_physical_matrix.mjs`
  - `run_cpp_client_synthetic_matrix.mjs`

`physical` 路径中：

- 禁用 `QOS_TEST_MATRIX_PROFILE`
- 只依赖真实 netem + 真实 stats/trace

`synthetic` 路径中：

- 允许 `QOS_TEST_MATRIX_PROFILE`
- 但 case 字段必须改名为显式 synthetic 语义

## Case Schema Realignment

### Physical Schema

保留：

- `bandwidth`
- `rtt`
- `loss`
- `jitter`

新增可选说明字段：

- `networkNote`
- `realWorldAnalogy`

禁止：

- 在 physical schema 中再解释出隐藏的 synthetic 等价值

### Synthetic Schema

新字段建议：

- `reportedRttMs`
- `lossRate`
- `sendCeilingBps`
- `jitterMs`
- `qualityLimitationReason`

禁止：

- 继续复用 `bandwidth/rtt/loss/jitter` 作为 synthetic 输入字段名

## Report Realignment

### Physical Report

必须展示：

- 名义 case 参数
- runner 施加的物理 netem 参数
- 真实观测 stats
- 现实网络类比说明

### Synthetic Report

必须展示：

- synthetic 输入
- 期望状态与动作
- deterministic 实测 trace

并在标题或摘要中明确标注：

- `synthetic`

## Migration Strategy

### Phase 1: Labeling And Freeze

- 冻结新增隐式变换
- 给现有 runner 打标签：
  - browser current matrix = `semi-physical legacy`
  - cpp current matrix = `synthetic-assisted legacy`
- 文档中明确这些入口不再用于现实网络签收

#### Phase 1 Deliverables

- 在 runner 入口、状态页或报告头部中加入 legacy 标签
- 输出一份 transformation inventory，至少列出：
  - browser netem 带宽折算
  - synthetic RTT 放大
  - cpp `B3` RTT 特判
  - synthetic `qualityLimitationReason`
  - synthetic `sendCeilingBps`

#### Phase 1 Non-Goals

- 不改变现有 legacy runner 行为
- 不承诺 legacy runner 与物理网络一一对应

### Phase 2: Physical Baseline Set

- 先迁移 `B1~B5`
- 把 baseline 家族改成真正物理可解释
- 以 browser physical runner 为第一批落地点

#### Phase 2 Scope

- 新建 browser physical baseline runner，或在 browser runner 中新增明确 physical 模式入口
- baseline 仅覆盖 `B1~B5`
- 不要求本 phase 覆盖 cpp physical runner

#### Phase 2 Physical Case Contract

每个 baseline case 必须只包含物理语义字段：

- `bandwidth`
- `rtt`
- `loss`
- `jitter`
- `networkNote`
- `realWorldAnalogy`

不允许出现：

- `reportedRttMs`
- `sendCeilingBps`
- synthetic `qualityLimitationReason`

#### Phase 2 Browser Runner Rules

- `bandwidth` 直接映射到 netem rate，不再打 `0.7 / 0.85`
- `rtt` 只做物理上必要的单向拆分，不再改变其报告语义
- `loss`、`jitter` 原样施加
- 观测值只来自真实浏览器 trace / stats
- baseline contamination 只允许作为基础设施异常保护，不得覆盖 baseline oracle

#### Phase 2 Report Contract

Physical baseline report 每个 case 至少展示三层：

1. `configured`
   - case 定义中的物理网络值
2. `applied`
   - 实际 netem 施加值
3. `observed`
   - 浏览器真实观测到的关键 stats 与状态

建议新增字段：

- `configured network`
- `applied netem`
- `observed baseline state`
- `observed impairment peak`
- `realWorldAnalogy`

#### Phase 2 Acceptance Style

Phase-2 baseline suite 只要求：

- 结果方向与严重度区间合理
- 现实网络解释一致

不要求：

- 精确 deterministic action 数
- synthetic 推导出的固定 RTT

### Phase 3: CPP Split

- 将 cpp matrix 拆成 physical / synthetic 两条
- 保留 synthetic 入口服务控制器回归

### Phase 4: Sweep Family Migration

- 依次迁移：
  - `BW*`
  - `L*`
  - `R*`
  - `J*`
  - `T*`
  - `S*`
  - `M*`
  - `O*`

## Risks

### Risk 1: 历史报告不可比

缓解：

- 新 artifact family
- 报告中保留 legacy 标签

### Risk 2: 物理 E2E 稳定性下降

缓解：

- 保留 synthetic runner 用于 deterministic regression
- 不再要求 physical runner 兼顾 deterministic oracle 角色

### Risk 3: 短期维护成本上升

缓解：

- 先从 baseline 家族迁移
- 逐步扩展而非一次性重写全部 48 case

### Risk 4: Physical runner 结果波动导致签收不稳定

缓解：

- baseline suite 先采用区间型 oracle
- 将 deterministic 要求保留给 synthetic suite
- 报告显式分离“物理表现”与“控制器回归”

## Decision Summary

1. **必须拆分两套测试体系**
2. **physical runner 的参数必须字面即语义**
3. **synthetic runner 可以保留，但必须显式承认 synthetic 输入**
4. **browser / cpp 不能再共享一种“半物理半合成”的含混口径**
5. **Phase 1 先冻结 legacy mixed runner，Phase 2 先从 `B1~B5` 做 browser physical baseline suite**
