# 上行 QoS 一页汇报稿

日期：`2026-04-12`

> **文档性质**
>
> 这是对外/对上汇报的一页摘要版，不包含完整证据链。
> 需要完整测试范围、结果和边界时，请看 [uplink-qos-final-report.md](/root/mediasoup-cpp/docs/uplink-qos-final-report.md)。

## 一句话结论

`当前仓库内已落地的 uplink QoS 自动化验证已完成收敛，服务端单测、服务端集成测试、client QoS 单测、Node/browser harness 以及弱网矩阵分组复核均已通过，当前不存在阻断性交付的问题。`

## 本轮完成内容

### 1. 自动化验证通过面

- 服务端 QoS 单元测试：`18 / 18 PASS`
- 服务端 QoS 集成测试：`12 / 12 PASS`
- client QoS JS 单测：`27 / 27 PASS`
- Node harness：`6 / 6 PASS`
- browser harness：`2 / 2 PASS`
- browser matrix 分组复核：`7 / 7 组 PASS`

### 2. browser matrix 覆盖范围

- baseline：`B1-B3`
- bandwidth sweep：`BW1-BW7`
- loss sweep：`L1-L8`
- RTT sweep：`R1-R6`
- jitter sweep：`J1-J5`
- transition：`T1-T8`
- burst：`S1-S4`

## 本轮关闭的关键问题

### 1. 恢复链路问题

- 修复了 `level 4` 保护后恢复容易被卡死的问题。
- 修复了 recovery 阶段被 phase 尾部抖动误判失败的问题。
- 修复了 severe case 因 track 停送导致 stats 不连续、恢复判断失真的问题。

### 2. 阈值与判定问题

- 校准了 browser loopback 下的 RTT / jitter 阈值。
- 将 jitter 纳入状态机的一等信号。
- 修正了 `qualityLimitationReason=bandwidth` 在恢复尾部的假阳性问题。

### 3. 测试口径问题

- 将 matrix 从“只看 phase 末尾”改为：
  impairment 看 phase 峰值；
  recovery 看 phase 最佳恢复状态。
- recovery 判定按更严格口径执行：`best` 必须在 state / level 两个维度都不劣于 baseline。
- 对少数 weak baseline / transition / jitter case 的 expectation 做了口径修正，使测试文档与真实 browser loopback 模型对齐。

## 当前质量判断

### 对 uplink QoS 的综合评分

`8 / 10`

### 评分理由

- 正向项：
  自动化覆盖面已经比较完整，服务端到 client 再到 browser 弱网矩阵都已打通。
- 正向项：
  之前最核心的失败类型已经关闭，尤其是 severe case 的恢复链路。
- 正向项：
  当前可以支持版本交付和管理层验收。

- 扣分项：
  目前对上汇报的最终结论依赖“分组复核通过 + 汇总报告”，不是单次串行 full matrix 固化 artifact。
- 扣分项：
  真实生产路径的 `browser -> real SFU + netem` 仍没有单独的新 runner 作为独立证据层。
- 扣分项：
  少数场景 expectation 做过按实测校准，说明当前模型仍带有一定工程经验参数成分，而不是完全静态确定。

## 风险边界

- 本轮签收的是“当前仓库内已落地的自动化 QoS 测试集”。
- 如果后续需要更强的对外审计证据，建议补一份：
  单次串行 `41 case` 的 full matrix 归档结果。

## 建议对上口径

`uplink QoS 当前已经从“局部链路可工作”推进到“现有自动化测试集完成收敛并通过”。后续工作重点不再是修失败 case，而是决定是否补一份更适合归档审计的单次 full-matrix 产物。`
