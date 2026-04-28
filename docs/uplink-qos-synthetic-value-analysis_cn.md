# 合成值网络代表性分析

> 核心问题：QoS 合成测试框架中注入的合成值能否真实代表此时的网络状况？

## 结论

**合成值能代表网络状况的"方向"和"严重程度等级"，不能代表"精确数值"和"动态行为"。**

| 能力 | 评估 |
|---|---|
| 区分"好网络"和"差网络" | ✅ 可以 |
| 触发正确的 QoS 状态转换方向 | ✅ 基本可以 |
| 精确复现真实 WebRTC stats 的数值 | ❌ 不能 |
| 复现拥塞控制算法的动态收敛行为 | ❌ 完全不能 |
| 复现码率的时序波动 | ❌ 不能（合成值是静态阶跃） |

---

## 逐信号分析

### 1. 丢包率（lossRate）— ✅ 能代表

**合成逻辑**（`synthetic_sweep_shared.mjs`）：

```js
lossRate: clamp(lossPct / 100, 0, 1)
```

**C++ 注入逻辑**（`client/main.cpp:818-820`）：

```cpp
const double lostDelta = (phase->lossRate / max(1e-9, 1.0 - phase->lossRate)) * sentDelta;
runtime.syntheticPacketsLost += llround(lostDelta);
```

合成丢包率直接等于 netem 施加的物理丢包率。C++ 注入公式确保 `lost/(lost+sent)` 等于目标丢包率。例如：发 100 包、丢包率 5% → `lostDelta = 0.05/0.95 × 100 ≈ 5.26` → `lost/(lost+sent) ≈ 5%`。

**评估：此信号忠实反映网络丢包状况。**

### 2. RTT — ⚠️ 方向正确，放大函数未标定

**合成逻辑**：

```js
const reportedRttMs = Math.round(baseRttMs + Math.min(baseRttMs, 100));
```

| 物理 netem delay | 合成 RTT | 放大倍率 |
|---|---|---|
| 25ms | 50ms | 2× |
| 55ms | 110ms | 2× |
| 100ms | 200ms | 2× |
| 180ms | 280ms | 1.56× |
| 350ms | 450ms | 1.29× |

**合理性**：真实 WebRTC 的 `roundTripTime`（基于 RTCP SR/RR）包含编码/RTCP 处理开销，确实比物理 RTT 大。

- **低 RTT（<100ms）时放大 2×**：真实 WebRTC 在局域网/同城网络下，reported RTT 通常是物理 RTT 的 1.5-3×，2× 处于合理区间。
- **高 RTT（>100ms）时固定加 100ms**：高 RTT 场景下额外开销相对稳定，100ms 偏移是保守但合理的近似。

**评估：方向正确，但未用真实 WebRTC 数据标定，某些区间可能偏差较大。**

### 3. Jitter — ✅ 直传（但有细微差异）

```js
jitterMs  // 直接传入，无变换
```

合成 jitter 直接等于场景定义的物理 jitter。

**潜在问题**：真实 WebRTC 报告的 jitter 经过 RFC 3550 的指数平滑（`J = J + (|D(i-1,i)| - J) / 16`），通常比瞬时网络 jitter 更平滑。合成值直接使用原始值，可能比真实报告值更"尖锐"。

**评估：方向正确，但合成值可能比真实 WebRTC 报告值偏大。**

### 4. 发送码率（sendCeilingBps）— ❌ 最大偏差源

**合成逻辑**（`synthetic_sweep_shared.mjs`）：

```js
utilization = 0.98 - 0.45*bwStress - 0.15*lossStress - 0.2*rttStress - 0.35*jitterStress
bitrateBps = 900000 * utilization
```

其中 stress 归一化函数：

```js
normalizeBandwidthStress(bw)  = clamp((2500 - bw) / 2000, 0, 1)
normalizeLossStress(loss)     = clamp(loss / 10, 0, 1)
normalizeRttStress(rtt)       = clamp((rtt - 100) / 250, 0, 1)
normalizeJitterStress(jitter) = clamp((jitter - 10) / 60, 0, 1)
```

**与真实 WebRTC 拥塞控制的差异：**

| 维度 | 合成模型 | 真实 WebRTC CC（GCC/SendSideBWE） |
|---|---|---|
| 响应速度 | 瞬时阶跃 | 渐进收敛（5-30秒） |
| 丢包敏感度 | 线性（loss/10） | 乘法降（0.85× per loss event） |
| RTT 影响 | 线性降 utilization | 影响探测速率和收敛时间 |
| 带宽探测 | 无 | TWCC/REMB 持续探测 |
| 竞争流 | 不考虑 | 公平性机制 |

**具体示例——Case BW3（bandwidth=1000kbps）**：
- 合成模型：`bwStress = (2500-1000)/2000 = 0.75` → `utilization ≈ 0.64` → `bitrate ≈ 578kbps` → ×0.75 → **≈ 434kbps**
- 真实 WebRTC：在 1Mbps 链路上，GCC 会在 10-20 秒内收敛到约 700-850kbps（利用率 70-85%），然后因探测行为产生周期性波动

**评估：合成模型给出固定值，真实值有动态波动且绝对值可能差接近 2×。权重系数（0.45, 0.15, 0.2, 0.35）和归一化函数无标定依据。**

### 5. qualityLimitationReason — ⚠️ 简化但方向正确

```js
qualityLimitationReason: severity >= 0.85 || utilization < 0.55 ? 'bandwidth' : 'none'
```

二值判断在极端情况下正确，但在中间地带（severity 0.5-0.85, utilization 0.55-0.7）可能与真实编码器行为不一致。

**评估：极端场景准确，中间地带可能有误判。**

---

## 风险评估

### 低风险（合成测试结论基本可信的场景）

- QoS 算法的阈值设置有足够容错余量（如 `NETWORK_WARN_LOSS_RATE = 0.04`，合成 loss 偏差 ±20% 以内）
- 测试只关注状态转换的方向（如：丢包 10% 是否触发 congested）
- 丢包率和 RTT 的测试

### 高风险（合成测试结论可能与线上不同的场景）

- 算法对 utilization 绝对值敏感（`bandwidthLimited` 判断依赖 `bitrateUtilization < 0.65`）
- 需要验证算法在"中间地带"（轻微拥塞）的行为
- 需要验证恢复时序（合成值瞬时跳变 vs 真实 CC 渐进恢复）

---

## 改进建议

1. **标定 utilization 模型**：用真实 WebRTC 客户端在相同 netem 条件下跑 baseline，用观测值反向标定权重系数和归一化函数。这是最有价值的单一改进。

2. **添加时序模拟**：在 phase 切换时不要瞬时跳变码率，而是用指数衰减/增长模拟 CC 收敛行为，典型时间常数 5-15 秒。

3. **Jitter 平滑**：对合成 jitter 应用 RFC 3550 平滑滤波，使其更接近真实 WebRTC 报告值。

4. **RTT 放大函数标定**：收集真实 WebRTC 在不同物理 RTT 下的 `roundTripTime` 报告值，拟合更准确的放大函数。

5. **增加端到端验证测试**：选择 3-5 个典型场景，同时运行合成测试和真实 WebRTC 测试（或重放录制的真实 stats），对比 QoS 算法的输出是否一致。

---

## 相关代码位置

| 文件 | 内容 |
|---|---|
| `tests/qos_harness/synthetic_sweep_shared.mjs` | 合成值计算（`toSyntheticCondition`、stress 归一化） |
| `tests/qos_harness/run_cpp_client_matrix.mjs` | 测试 profile 构建（`buildMatrixTestProfile`）和 case 特殊处理 |
| `client/main.cpp:788-833` | C++ 侧合成值注入（`applyMatrixTestProfile`） |
| `client/qos/QosSignals.h` | 信号派生逻辑（`deriveSignals`、EWMA 平滑） |
| `client/qos/QosConstants.h` | 阈值常量（`NETWORK_WARN_LOSS_RATE`、`NETWORK_CONGESTED_UTILIZATION` 等） |
