# IPC 再生成与回归机制

这份文档只回答一个问题：

**什么时候必须重生成 FlatBuffers 头、重编主工程/worker，并跑对应回归。**

它不替代构建文档，也不替代测试文档。
它的目标是避免再次出现：

- `invalid FlatBuffers message`
- worker 与主工程 IPC 协议解释不一致
- 某一侧单独看起来正常，但两侧联调时崩

---

## 1. 结论先说

凡是可能影响 **mediasoup-cpp 主进程** 和 **mediasoup-worker 子树** 之间 IPC 二进制契约的改动，
都必须强制执行下面这套流程：

1. 明确判断这次改动是否属于 **schema / codegen / runtime 契约变更**，还是 **纯 framing / 解析逻辑变更**
2. 在需要时重生成主工程 `generated/*_generated.h`
3. 在需要时重建 worker FBS 生成产物
4. 重编主工程与 worker
5. 跑 IPC / transport / integration / qos 关键回归
6. 最终必须通过：

```bash
cd /root/workspace/mediasoup-cpp
./script/run_all_tests.sh all
```

不能靠“肉眼觉得这次改动应该不影响协议”来跳过。

---

## 2. 触发条件

下面任意一类改动，都视为 **必须触发再生成/重编/回归**。

### 2.1 直接改了 schema

涉及路径：

- `fbs/*.fbs`
- `src/mediasoup-worker-src/worker/fbs/*.fbs`

这类改动一定会改变：

- 字段名
- 字段顺序
- union body
- optional/null 表达
- 验证逻辑

所以必须触发完整流程。

### 2.2 改了 Channel/FlatBuffers IPC 解析或构造逻辑

涉及路径典型包括：

- `src/Channel.cpp`
- `src/Channel.h`
- `src/TransportConnectResponseUtils.h`
- `src/mediasoup-worker-src/worker/src/Channel/*`
- `src/mediasoup-worker-src/worker/include/Channel/*`

这类改动的风险是：

- framing 处理错误
- size prefix 处理错误
- `Message/Response/Notification` 顶层解释不一致
- 某些消息只在联调时才炸

这类改动即使没改 schema，也必须至少触发：

- 主工程 / worker 重编
- IPC 关键回归
- 单入口全量回归：

```bash
./script/run_all_tests.sh all
```

注意：

- **纯 framing / 解析逻辑变更** 不一定必须重生成 FlatBuffers 头
- 但 **绝不能跳过全量回归**

### 2.3 改了 worker 里会影响响应/通知 payload 的构造点

典型包括：

- `HandleRequest()`
- `FillBuffer()`
- `FillBufferStats()`
- `channelNotifier->Emit(...)`
- `request->Accept(...)`

常见文件位置：

- `src/mediasoup-worker-src/worker/src/RTC/*`

这类改动经常不会影响主工程编译，
但会直接影响 worker 发给主工程的 payload 结构。

### 2.4 改了 FlatBuffers 版本、include 路径或生成参数

涉及路径典型包括：

- `CMakeLists.txt`
- `setup.sh`
- worker `meson.build`
- worker `subprojects/flatbuffers.wrap`

这是高风险改动。

因为即使 `.fbs` 文本完全没变，
**不同版本 / 不同生成参数** 生成出来的 C++ API 和验证行为也可能不同。

---

## 3. 当前仓库现实状态

这份文档必须以 **当前仓库真实状态** 为准，而不是以某次排障时的中间假设为准。

当前仓库里：

- worker 子树固定在 FlatBuffers `24.3.6`
  - 位置：`src/mediasoup-worker-src/worker/subprojects/flatbuffers.wrap`
- 主工程当前使用的 runtime / generated headers 仍然来自：
  - `third_party/flatbuffers/include`
  - `setup.sh`
  - 当前 `generated/*_generated.h`

也就是说，**当前仓库不是“两边已经统一到同一 FlatBuffers 版本”的状态**。

这次真正修复 IPC 问题的关键根因是：

- 主进程 `Channel` 在读取 worker 消息时，错误剥掉了 inner `4-byte size prefix`
- worker 发的是 `size-prefixed Message`
- main 端原来却把剩余 payload 当成完整根对象去解

因此，当前最准确的工程规则不是：

- “一律以 worker 24.3.6 为 source of truth，主工程必须同版”

而是：

- **一切 IPC 风险判断，必须落到当前仓库的真实 runtime / generated headers / worker codegen / framing 逻辑上**
- **任何变更后，都必须靠单入口全量回归去证明双侧契约仍然成立**

---

## 4. 必跑流程

### 4.1 再生成

下面两种情况要区分：

#### A. schema / flatc / 生成参数发生变化

必须同时做两件事：

1. 主工程重新生成：

- `generated/*_generated.h`

2. worker 重新生成：

- `worker/out/.../fbs/FBS/*`

不能只生成一边。

#### B. 纯 Channel framing / 解析逻辑变更

这类情况：

- 不一定必须 regenerate
- 但必须重编主工程与 worker
- 并且必须通过单入口全量回归

### 4.2 重编

至少重编：

- worker
- `mediasoup_lib`
- `mediasoup-sfu`

如果涉及测试路径，测试二进制也要一起重编：

- `mediasoup_tests`
- `mediasoup_integration_tests`
- `mediasoup_qos_integration_tests`
- `mediasoup_qos_accuracy_tests`

### 4.3 关键回归

最少要跑这几类：

1. Channel / transport validation

- `mediasoup_tests`
  - `TransportConnectValidationTest.*`
  - `OwnedResponseTest.*`

2. 普通集成链

- `mediasoup_integration_tests`
  - 至少覆盖 join / createTransport / producer close / recvTransport replace

3. QoS 集成链

- `mediasoup_qos_integration_tests`
  - 至少覆盖 `plainPublish` / `getStats`

4. QoS accuracy

- `mediasoup_qos_accuracy_tests`

5. 最终验收

```bash
cd /root/workspace/mediasoup-cpp
./script/run_all_tests.sh all
```

---

## 5. 为什么不能只看单侧编译

这类问题的典型陷阱是：

- 主工程单独编译通过
- worker 单独编译通过
- 某个单测也可能通过
- 但两边一跑就出现：
  - `invalid FlatBuffers message`
  - `Channel closed`
  - `getStats` 空
  - close/cleanup 通知丢失

原因是：

**这不是单侧代码正确性问题，而是双侧 IPC 契约一致性问题。**

所以这条机制不能依赖“看起来改动不大”，必须依赖明确触发规则。

---

## 6. 推荐工程化约束

只靠人记住不够，建议加三层自动化约束。

### 6.1 生成期 manifest / stamp

记录：

- `flatc --version`
- `flatc` 路径
- 生成参数
- `fbs/*.fbs` hash
- 主工程 flatbuffers include 路径
- worker flatbuffers include 路径

当前仓库已落地的自动化文件：

- `scripts/ipc_contract_guard.py`
- `docs/generated/ipc-contract-manifest.json`
- `docs/generated/ipc-full-regression-stamp.json`

用途：

- 记录当前 IPC 敏感文件 fingerprint
- 记录最近一次成功跑完：

```bash
./script/run_all_tests.sh all
```

时对应的 IPC 合同快照

### 6.2 构建前 fail-fast

当前已落地：

- `scripts/run_all_tests.sh` 在执行前会跑：

```bash
python3 scripts/ipc_contract_guard.py check-consistency
```

- `build_image.sh` / `scripts/package_image.sh` 在构建镜像前会跑：

```bash
python3 scripts/ipc_contract_guard.py verify-release-readiness
```

它会检查：

- 主工程 generated headers 与主工程 runtime 头版本是否自洽
- worker wrap 版本与 worker 已生成头版本是否自洽
- 当前 IPC 敏感文件 fingerprint 是否仍然匹配最近一次成功全量回归的 stamp

如果不匹配，会直接拒绝构建镜像，逼迫先重跑单入口全量回归。

### 6.3 CI / 入口触发规则

如果改动命中：

- `fbs/**`
- `src/Channel*`
- `src/TransportConnectResponseUtils.h`
- `src/mediasoup-worker-src/worker/src/Channel/**`
- `src/mediasoup-worker-src/worker/src/RTC/**`
- `src/mediasoup-worker-src/worker/fbs/**`
- `CMakeLists.txt`
- `setup.sh`

则 CI 必须强制跑：

```bash
./script/run_all_tests.sh all
```

当前本地入口也已经强制这样做：

- 只要要产出正式镜像，就必须先通过 `verify-release-readiness`
- 而 `verify-release-readiness` 的前提就是之前已经成功跑过 `./script/run_all_tests.sh all`

---

## 7. 最短操作建议

如果你刚改完一处 IPC 相关逻辑，不想判断太多，最稳妥做法就是直接执行：

1. 如果改动涉及 schema / flatc / codegen，先 regenerate
2. 重编 worker 和主工程
3. 跑：

```bash
cd /root/workspace/mediasoup-cpp
./script/run_all_tests.sh all
```

4. 成功后确认：

- `docs/generated/ipc-contract-manifest.json`
- `docs/generated/ipc-full-regression-stamp.json`

已经更新

如果这套太慢，再考虑细分增量回归；但**正式镜像构建前**仍然必须让单入口全量回归 stamp 覆盖当前 IPC 敏感改动。

---

## 8. 一句话规则

**凡是可能改变 worker 发给主工程的 FlatBuffers 二进制布局、顶层 framing、字段访问器、union/body 结构、验证语义的改动，都必须触发“必要时再生成 + 必定重编 + IPC 回归 + 单入口全量回归”，并更新 IPC regression stamp。**
