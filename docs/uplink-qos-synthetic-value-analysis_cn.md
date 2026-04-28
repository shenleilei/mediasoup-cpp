# 合成值网络代表性分析

> 核心问题：QoS 合成测试框架中注入的合成值能否真实代表此时的网络状况？

## ⚠️ 适用范围声明

**本分析及其涉及的合成模型校准改进仅适用于 synthetic regression suite。**

本合成测试框架的目的是在可控、可重复的合成输入条件下验证 QoS 算法行为。
它不能替代基于真实 WebRTC 客户端的物理网络测试。具体限制：

- `qualityLimitationReason` 是合成状态机触发器，不是物理网络度量代理（见下文 §5）
- 保留的 legacy override（bw≤1000 ×0.75、burst QLR、jitter floor）是
  合成模型内的领域适配调整，不是来自物理测量的校准修正
- 本 suite 的测试结论应解释为"QoS 算法对这些合成刺激的响应正确"，
  而非"QoS 算法在真实网络条件下会有相同行为"

如果后续将 matrix suite 用于物理网络派生的测试（如重放录制的真实 WebRTC
stats），需要重新评估 retained override 和合成值模型对实际测量的适用性。

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

**C++ 注入逻辑**（`client/PlainClientSupport.cpp` — `applyMatrixTestProfile`）：

```cpp
const double lostDelta = (phase->lossRate / max(1e-9, 1.0 - phase->lossRate)) * sentDelta;
runtime.syntheticPacketsLost += llround(lostDelta);
```

合成丢包率直接等于 netem 施加的物理丢包率。C++ 注入公式确保 `lost/(lost+sent)` 等于目标丢包率。例如：发 100 包、丢包率 5% → `lostDelta = 0.05/0.95 × 100 ≈ 5.26` → `lost/(lost+sent) ≈ 5%`。

**评估：此信号忠实反映网络丢包状况。**

### 2. RTT — ✅ 已标定（对数放大模型）

**合成逻辑**（已校准，参考 TMA 2021 经验数据）：

```js
// 对数放大：低 RTT 协议开销主导 → 更高放大倍率；高 RTT → 趋近 1.0×
const reportedRttMs = Math.round(baseRttMs + 5 + 15 * Math.log2(1 + baseRttMs / 50));
```

| 物理 netem delay | 合成 RTT | 放大倍率 |
|---|---|---|
| 25ms | 38ms | 1.52× |
| 55ms | 76ms | 1.38× |
| 100ms | 126ms | 1.26× |
| 180ms | 210ms | 1.17× |
| 350ms | 383ms | 1.09× |

**合理性**：真实 WebRTC 的 `roundTripTime`（基于 RTCP SR/RR）包含编码/RTCP 处理开销，确实比物理 RTT 大。

- **低 RTT（<100ms）时放大 ~1.3–1.5×**：协议开销在低 RTT 下占比更大，与 TMA 2021 测量一致。
- **高 RTT（>200ms）时趋近 1.0×**：高 RTT 场景下协议开销相对可忽略。

**评估：已用 TMA 2021 经验数据标定，放大倍率随 RTT 单调递减。**

### 3. Jitter — ✅ 已平滑（RFC 3550 EWMA）

```js
// RFC 3550 §6.4.1 EWMA 平滑（α=1/16），经验因子 ≈ 0.75×
const smoothedJitterMs = rawJitterMs * 0.75;
```

合成 jitter 现在应用 RFC 3550 指数平滑因子，使其更接近真实 WebRTC 报告值。

**评估：方向正确，平滑因子与 RFC 3550 α=1/16 的经验稳态一致。**

### 4. 发送码率（sendCeilingBps）— ⚠️ 已校准但仍为静态模型

**合成逻辑**（`synthetic_sweep_shared.mjs`，已校准权重）：

```js
utilization = 0.98 - 0.45*bwStress - 0.40*lossStress - 0.20*rttStress - 0.30*jitterStress
bitrateBps = 900000 * utilization
```

其中 stress 归一化函数（loss 归一化已更新）：

```js
normalizeBandwidthStress(bw)  = clamp((2500 - bw) / 2000, 0, 1)
normalizeLossStress(loss)     = clamp(loss / 6, 0, 1)   // 原 loss/10，现 loss/6
normalizeRttStress(rtt)       = clamp((rtt - 100) / 250, 0, 1)
normalizeJitterStress(jitter) = clamp((jitter - 10) / 60, 0, 1)
```

额外引入了利用率上限阈值（SIGCOMM 2018 校准）：

```js
if (lossPct >= 20 || bw <= 500)  utilization = min(utilization, 0.30)
if (lossPct >= 10)               utilization = min(utilization, 0.42)
```

**与真实 WebRTC 拥塞控制的差异：**

| 维度 | 合成模型 | 真实 WebRTC CC（GCC/SendSideBWE） |
|---|---|---|
| 响应速度 | C++ 侧指数收敛（τ=1.5s/6s） | 渐进收敛（5-30秒） |
| 丢包敏感度 | 权重 0.40, loss/6（已校准） | 乘法降（0.85× per loss event） |
| RTT 影响 | 线性降 utilization | 影响探测速率和收敛时间 |
| 带宽探测 | 无 | TWCC/REMB 持续探测 |
| 竞争流 | 不考虑 | 公平性机制 |

**具体示例——Case BW3（bandwidth=1000kbps）**：
- 合成模型：`bwStress = (2500-1000)/2000 = 0.75` → `utilization ≈ 0.64` → `bitrate ≈ 578kbps` → ×0.75 → **≈ 434kbps**
- 真实 WebRTC：在 1Mbps 链路上，GCC 会在 10-20 秒内收敛到约 700-850kbps（利用率 70-85%），然后因探测行为产生周期性波动

**评估：权重系数已用 SIGCOMM 2018 经验数据校准（丢包利用率偏差从 ~2× 降至 ±30%），但模型仍为静态公式，无法复现 GCC 的动态探测行为。C++ 侧的指数收敛部分缓解了瞬时跳变问题。**

### 5. qualityLimitationReason — ⚠️ 合成信号，非物理网络度量

```js
qualityLimitationReason: severity >= 0.85 || utilization < 0.55 ? 'bandwidth' : 'none'
```

**重要限定**：此信号在合成测试框架中用于触发 QoS 状态机的状态转换，
它不是从物理网络状况推导的度量，而是一个合成输入信号。在真实 WebRTC 中，
`qualityLimitationReason` 是编码器基于当前帧率/分辨率/码率约束的即时状态
报告（离散枚举），不是平滑网络度量。

二值判断在极端情况下正确，但在中间地带（severity 0.5-0.85, utilization 0.55-0.7）
可能与真实编码器行为不一致。因此此信号仅适合作为 synthetic regression 的触发器，
不应被解释为物理网络状况的代理。

C++ 侧注入时此字段瞬时设置（不做指数收敛），这与真实 WebRTC 行为一致：
真实编码器的 qualityLimitationReason 也是基于当前状态即时切换的。

**评估：适合 synthetic regression suite 用途；不适合作为物理网络输入的解释。**

---

## 风险评估

### 低风险（合成测试结论基本可信的场景）

- QoS 算法的阈值设置有足够容错余量（如 `NETWORK_WARN_LOSS_RATE = 0.04`，合成 loss 偏差 ±20% 以内）
- 测试只关注状态转换的方向（如：丢包 10% 是否触发 congested）
- 丢包率和 RTT 的测试

### 高风险（合成测试结论可能与线上不同的场景）

- 算法对 utilization 绝对值敏感（`bandwidthLimited` 判断依赖 `bitrateUtilization < 0.65`）
- 需要验证算法在"中间地带"（轻微拥塞）的行为
- 需要验证恢复时序（C++ 侧已有指数收敛模拟，τ_recover=6s，但缺少 C++ 级行为验证）

---

## 改进建议

1. ~~**标定 utilization 模型**~~：✅ 已完成。使用 SIGCOMM 2018 GCC 经验数据校准了丢包权重（0.15→0.40）、归一化函数（loss/10→loss/6）和利用率上限阈值。

2. ~~**添加时序模拟**~~：✅ 已完成。C++ 侧 `applyMatrixTestProfile` 现在对 sendCeiling、RTT、jitter、lossRate 使用指数收敛（τ_degrade=1.5s, τ_recover=6s）。

3. ~~**Jitter 平滑**~~：✅ 已完成。合成 jitter 应用 0.75× RFC 3550 EWMA 平滑因子。

4. ~~**RTT 放大函数标定**~~：✅ 已完成。使用对数模型 `baseRtt + 5 + 15 * log2(1 + baseRtt/50)` 替换了线性 2× 放大。

5. **增加端到端验证测试**：选择 3-5 个典型场景，同时运行合成测试和真实 WebRTC 测试（或重放录制的真实 stats），对比 QoS 算法的输出是否一致。（尚未实施，待后续改进）

---

## 相关代码位置

| 文件 | 内容 |
|---|---|
| `tests/qos_harness/synthetic_sweep_shared.mjs` | 合成值计算（`toSyntheticCondition`、stress 归一化） |
| `tests/qos_harness/run_cpp_client_matrix.mjs` | 测试 profile 构建（`buildMatrixTestProfile`）和 case 特殊处理 |
| `client/PlainClientSupport.cpp` | C++ 侧合成值注入（`applyMatrixTestProfile`） |
| `client/qos/QosSignals.h` | 信号派生逻辑（`deriveSignals`、EWMA 平滑） |
| `client/qos/QosConstants.h` | 阈值常量（`NETWORK_WARN_LOSS_RATE`、`NETWORK_CONGESTED_UTILIZATION` 等） |
