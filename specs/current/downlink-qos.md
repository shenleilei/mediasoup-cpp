# 下行服务质量 (Downlink QoS)

## 已存储的订阅者快照 (Stored Subscriber Snapshots)

- 被接受的 `downlinkClientStats` 快照**必须 (SHALL)** 能够通过 `getStats` 接口作为 `downlinkClientStats` 字段被可见。
- 对于 `downlinkClientStats` 来说，“被接受 (accepted)” 意味着请求的响应返回了 `ok=true` 并且带有 `data.stored=true`。
- `downlinkClientStats.subscriptions` **必须 (SHALL)** 保留有效且一致的 consumer / producer 映射关系，并且在存储前**必须 (SHALL)** 丢弃不一致的映射。

## 异步下行健康度 (Async Downlink Health)

- `downlinkQos.health` 和 `downlinkQos.degradeLevel` 是由 Worker 侧的下行调度规划状态推导而来的。
- 调用方**必须 (SHALL)** 将 `downlinkQos` 视为与刚刚被接受的 `downlinkClientStats` 最终一致的数据，而不能将其当作同步请求的响应字段来看待。

## 预算紧缺时的优先级 (Tight-Budget Priority)

- 当某个订阅者的总入网带宽（incoming bitrate）有限，并且同时存在可见的屏幕共享（screen-share）和可见的网格视频（grid）进行竞争时，分配给屏幕共享的带宽**严禁 (SHALL NOT)** 低于分配给网格视频的带宽。
- 在预算仅能容纳一条基础层（base-layer）视频消费者时，屏幕共享消费者**必须 (SHALL)** 优先于可见的网格视频消费者被准入（admitted）。