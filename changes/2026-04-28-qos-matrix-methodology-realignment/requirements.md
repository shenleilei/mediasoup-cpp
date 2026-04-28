# Requirements

## Summary

重构当前 QoS matrix 测试方法论，使测试输入参数能够直接映射到现实物理网络语义，并把“物理链路端到端验证”和“合成控制器回归验证”明确拆分为两套不同职责的测试体系。

## Problem Statement

当前 browser / cpp matrix 同时混合了：

- 物理链路模拟（`tc netem`）
- synthetic 条件变换（RTT 放大、带宽 utilization 推导、`qualityLimitationReason` 注入）
- runner 特判（例如 `B3` 的 cpp 额外 RTT 抬升）
- 基础设施污染 heuristic

结果是：

- case 名义参数与真实施加/真实判定参数不一致
- 同一个 case 在 browser 与 cpp runner 中的语义不一致
- 测试结果难以向现实网络条件解释
- matrix 更适合作为内部回归探针，而不适合作为“真实线上网络映射”的签收依据

## Business Goal

建立一套可解释、可签收、可持续维护的 QoS 验证体系，使团队能够：

- 直接用 case 参数解释现实物理网络场景
- 区分“真实端到端表现”和“控制器状态机回归”
- 在不牺牲回归稳定性的前提下，恢复测试口径透明度

## In Scope

- 重新定义 QoS 测试体系边界：
  - `physical-e2e matrix`
  - `synthetic-controller matrix`
- 为现有 mixed runner 定义 legacy 冻结口径
- 为 baseline 家族 `B1~B5` 设计第一批 `physical-e2e` suite
- 明确 case 参数语义：
  - case 值
  - 实际 netem 值
  - 控制器输入值
- 移除或淘汰当前物理 matrix 中的隐式参数放大/折算策略
- 重新定义 browser / cpp runner 的职责
- 重新定义 artifact / report 的标识与说明口径
- 为 baseline / sweep / transition / burst / oscillation 等 case 家族定义迁移规则
- 为历史 case（尤其 `B1~B5`、`L*`、`R*`、`T*`）定义改造策略

## Out Of Scope

- 不在本 change 中直接实现全部 runner 改造
- 不在本 change 中直接改写全部 48 个 case 的最终数值
- 不在本 change 中重写 QoS 控制器本身
- 不在本 change 中引入新的 WebRTC 或 mediasoup 底层算法

## User Stories

### Story 1

作为开发者，我希望 `B3` 这种 case 的参数可以直接解释为现实网络条件，而不是经过多层隐式变换后才决定控制器输入。

### Story 2

作为测试维护者，我希望 browser 与 cpp matrix 的职责边界明确，这样失败时能够判断是“产品逻辑问题”还是“harness 建模问题”。

### Story 3

作为评审/签收人，我希望报告中每一个 case 都能回答“它大概对应现实中的什么网络场景”。

## Acceptance Criteria

### Requirement 1: 测试体系职责分离

系统 SHALL 将当前 matrix 验证拆分为两套明确职责的体系：

- `physical-e2e matrix`
- `synthetic-controller matrix`

#### Scenario: 阅读任一测试入口

- WHEN 维护者查看某个 runner 的入口脚本和报告
- THEN 能明确判断该 runner 属于物理端到端验证还是合成控制器回归

### Requirement 2: 参数字面即物理语义

物理 E2E matrix SHALL 保证 case 参数字面即实际物理网络语义。

#### Scenario: Physical B3

- WHEN case 定义写 `bandwidth=2000, rtt=55, loss=0.5, jitter=12`
- THEN 施加到物理链路和用于解释报告的参数 SHALL 直接对应该语义
- AND SHALL NOT 再通过隐式 RTT 放大、带宽折扣、case 特判等方式改变其测试意义

### Requirement 3: Synthetic 输入显式标注

synthetic-controller matrix SHALL 明确声明其输入是合成控制器输入，而非现实网络值。

#### Scenario: Synthetic B3

- WHEN synthetic runner 仍需要使用放大后的 RTT 或推导出的 `sendCeilingBps`
- THEN 这些值 SHALL 在 case 定义或生成报告中被显式标明
- AND SHALL NOT 与物理 case 参数混用为同一含义

### Requirement 4: Browser 与 CPP 口径一致性

browser 与 cpp 的同类 runner SHALL 共享同一方法论口径。

#### Scenario: Physical browser/cpp

- WHEN browser 与 cpp 都声明自己是 `physical-e2e`
- THEN 两者 SHALL 使用同一套“参数字面即物理语义”的规则

#### Scenario: Synthetic browser/cpp

- WHEN browser 或 cpp 声明自己是 `synthetic-controller`
- THEN 两者 SHALL 显式接受 synthetic 输入口径，并在报告中标注

### Requirement 5: 迁移兼容

系统 SHALL 提供迁移期兼容方案，避免现有报告与脚本一次性失效。

#### Scenario: Migration period

- WHEN 新体系开始落地
- THEN 旧 matrix 入口 SHALL 有明确的弃用说明、替代入口和迁移时间表

### Requirement 6: Legacy Freeze

系统 SHALL 冻结当前 mixed runner 的方法论语义，禁止继续扩大其职责范围。

#### Scenario: Existing runner labeling

- WHEN 维护者查看当前 `run_matrix.mjs` 或 `run_cpp_client_matrix.mjs`
- THEN 文档或报告 SHALL 明确标注其为 legacy mixed/synthetic-assisted 入口
- AND SHALL NOT 再将其作为现实物理网络签收口径引用

### Requirement 7: Phase-2 Physical Baseline Suite

系统 SHALL 先以 `B1~B5` 作为第一批 physical baseline suite 落地点。

#### Scenario: Physical baseline suite

- WHEN baseline suite case 定义写入 `bandwidth/rtt/loss/jitter`
- THEN browser physical runner SHALL 直接按这些值施加 netem
- AND report SHALL 同时展示 configured / applied / observed 三层信息

### Requirement 8: Physical Runner Input Purity

physical runner SHALL 禁止 synthetic shaping。

#### Scenario: Browser physical baseline

- WHEN browser physical baseline suite 运行
- THEN SHALL NOT 使用：
  - 带宽折算
  - RTT 放大
  - synthetic `qualityLimitationReason`
  - case-specific synthetic 特判

## Non-Functional Requirements

- Clarity: 任一 case 的现实含义必须可解释
- Traceability: 任一失败必须能追溯到 runner 层、物理层或控制器层
- Maintainability: 后续新增 case 不允许再次引入隐式变换而不文档化
- Comparability: 新旧体系在迁移期内必须可并行对比

## Open Questions

- 物理 E2E matrix 是否仍保留 `browser` 与 `cpp` 两条入口，还是只保留各自最有代表性的入口？
- synthetic-controller matrix 是否应完全脱离 `tc netem`，只保留 deterministic stats/profile 注入？
- 旧的 `docs/generated/*matrix-report*.json` 是否需要按 runner 家族拆成新的 artifact family？
