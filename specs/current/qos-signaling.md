# QoS 信令 (QoS Signaling)

## Worker 侧完成的统计响应 (Worker-Completed Stats Responses)

- `clientStats` 和 `downlinkClientStats` **必须 (SHALL)** 在其归属的 `WorkerThread` 将请求处理到足以确定存储结果之后，才发送响应。
- 成功的 QoS 统计响应**必须 (SHALL)** 包含 `data.stored` 字段。
- 当 Worker 侧的注册中心（registry）故意忽略一个结构上有效的快照时，响应**必须 (SHALL)** 保持 `ok=true`，并且**必须 (SHALL)** 包含 `data.stored=false` 以及 `data.reason` 字段。
- 当运行时前置条件在 Worker 侧处理完成前消失时，响应**必须 (SHALL)** 返回失败，而不能继续报告成功。

## 存储结果语义 (Store Outcome Semantics)

- `data.stored=true` 意味着该快照已被接受并存入 Worker 侧的 QoS 注册中心。
- `data.stored=false` 意味着请求已被处理但快照未被存储，例如，因为相对于注册中心中已存在的条目来说，该快照已经是过期的数据（stale）。