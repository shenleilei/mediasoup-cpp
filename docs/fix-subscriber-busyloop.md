# Fix: RoomRegistry subscriberLoop CPU 100% Busy Loop

## 问题现象

`mediasoup-sfu` 进程 CPU 占用持续 99%+，通过 `top -H` 定位到单个线程（subscriber 线程）占满 CPU。

## 根因分析

### perf 火焰图定位

使用 `perf record -F 99 -p <pid> -g -o perf_sfu.data -- sleep 10` 采集 CPU 样本，结果显示几乎所有采样都落在：

- `mediasoup::RoomRegistry::subscriberLoop` (~15%)
- `redisGetReply` / `redisReaderGetReply` (~70%)
- `__errno_location` (~5%)

### 代码分析

问题在 `src/RoomRegistry.h` 的 `subscriberLoop()` 中：

```cpp
// 设置 2 秒读超时
struct timeval rtv = {2, 0};
redisSetTimeout(sub, rtv);

while (!subStop_) {
    redisReply* msg = nullptr;
    int rc = redisGetReply(sub, (void**)&msg);
    if (rc != REDIS_OK || !msg) {
        if (sub->err == REDIS_ERR_IO &&
            (errno == EAGAIN || errno == EWOULDBLOCK))
            continue; // ← 问题在这里
    }
}
```

### hiredis 超时行为的坑

1. `redisSetTimeout` 设置 SO_RCVTIMEO，2 秒无数据时 `read()` 返回 -1，errno = EAGAIN
2. hiredis 内部将 `context->err` 设为 `REDIS_ERR_IO`，**此状态是持久的**
3. 后续 `redisGetReply` 调用检测到 `context->err != 0`，**立即返回错误而不阻塞**
4. 代码 `continue` 回到循环顶部，再次调用 `redisGetReply`，再次立即返回 → **busy loop**

这是 hiredis 的已知行为但文档未明确说明：超时后 context 进入不可恢复的错误状态。

## 修复方案

用 `poll()` 替代 `redisSetTimeout` 来控制超时：

```cpp
// 不设置 redisSetTimeout，fd 设为非阻塞
int flags = fcntl(sub->fd, F_GETFL, 0);
fcntl(sub->fd, F_SETFL, flags | O_NONBLOCK);

while (!subStop_) {
    struct pollfd pfd = {sub->fd, POLLIN, 0};
    int ret = poll(&pfd, 1, 2000);  // 2s 超时
    if (ret == 0) continue;          // 超时，检查 subStop_
    if (ret < 0) break;              // poll 错误

    redisReply* msg = nullptr;
    int rc = redisGetReply(sub, (void**)&msg);
    // ...
}
```

### 为什么选择 poll 方案

| 方案 | 优点 | 缺点 |
|------|------|------|
| `continue` → `break` 重连 | 改动最小 | 每次超时都重连+syncAll，增加 Redis 压力 |
| 手动清除 `sub->err` | 不需要重连 | 修改 hiredis 内部状态，不规范 |
| **poll + 非阻塞 fd** | 不污染 context，不重连 | 改动稍多 |

## 影响范围

- 仅修改 `src/RoomRegistry.h` 的 `subscriberLoop()` 方法
- 不影响 Redis pub/sub 功能逻辑
- 不影响其他 Redis 连接（命令连接使用独立 context）

## 验证

1. 编译通过
2. 部署后观察 `mediasoup-sfu` 进程 CPU 应从 99% 降至接近 0%（空闲时）
3. Redis pub/sub 消息收发正常
