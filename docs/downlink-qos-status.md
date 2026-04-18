# Downlink QoS 当前状态

> 文档性质
>
> 这是 downlink QoS 的稳定摘要入口。
> 它回答当前范围、当前结果入口以及哪些内容还不在已完成口径里。

## 1. 当前范围

当前仓库里的 downlink QoS，主范围是：

- subscriber receive control
- hidden / visible / size / priority 驱动的 consumer-side 控制
- downlink health-based degrade / recovery
- publisher-side zero-demand `pauseUpstream / resumeUpstream` 协调

明确还不在当前已完成口径里：

- `dynacast`
- room-level global bitrate budgeting

## 2. 当前结果入口

downlink 当前的运行结果仍以“机器结果页 + generated artifact”为主，而不是单独的最终签收报告。

入口如下：

- 当前机器汇总页：
  [downlink-qos-test-results-summary.md](./downlink-qos-test-results-summary.md)
- 当前机器结果页：
  [downlink-qos-case-results.md](./downlink-qos-case-results.md)
- full / targeted generated artifact：
  [generated/downlink-qos-matrix-report.json](./generated/downlink-qos-matrix-report.json)
  [generated/downlink-qos-matrix-report.targeted.json](./generated/downlink-qos-matrix-report.targeted.json)

注意：

- [downlink-qos-test-results-summary.md](./downlink-qos-test-results-summary.md) 反映最近一次 `scripts/run_qos_tests.sh` 的 downlink 汇总结果。
- [downlink-qos-case-results.md](./downlink-qos-case-results.md) 反映最近一次 full `downlink-matrix` 详细 case 渲染结果。

## 3. 设计与实现文档

如果要继续做 downlink QoS，建议按这个顺序看：

1. [downlink-qos-design_cn.md](./downlink-qos-design_cn.md)
2. [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md)
3. [downlink-qos-v3-design_cn.md](./downlink-qos-v3-design_cn.md)
4. [downlink-qos-v3-implementation-plan_cn.md](./downlink-qos-v3-implementation-plan_cn.md)
5. [qos-test-coverage_cn.md](./qos-test-coverage_cn.md)

## 4. 当前对外口径

更稳妥的说法是：

`当前仓库已经具备 downlink QoS 的 subscriber-side 控制与 zero-demand publisher supply 协调能力；dynacast 与更高层的全局供给预算仍属后续范围。`
