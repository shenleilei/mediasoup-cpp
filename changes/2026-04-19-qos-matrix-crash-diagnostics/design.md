## Context

当前 browser matrix 首故障只留下了“症状层”日志：

- Puppeteer detached frame
- browser disconnected
- browser process exit signal

但仍缺少根因层证据：

- 当前浏览器 pid 树与 cgroup 归属
- 浏览器 stderr/stdout
- 与当前 case 时间窗对齐的内核日志
- 长时 phase 中更近场的资源快照
- 当前 case 与系统中其他长期残留 `headless_shell` 的区分

因此 full run 失败后仍不能明确回答：

- 是内核 OOM kill
- 是 browser 自身 crash
- 是外部进程 kill
- 还是 runner / esbuild 自身先失效

## Goal

增强 browser matrix 诊断，使一次 full run 失败后能直接识别首个失效组件和更可信的系统证据来源。

本 change 不修改 QoS 判定逻辑，不尝试修复业务回归。

## Proposed Approach

### 1. 聚焦当前 browser 进程树

在 `loopback_runner.mjs` 中对当前 `browserPid` 采集：

- `/proc/<pid>/status`
- `/proc/<pid>/cgroup`
- 以当前 `browserPid` 为根的子进程树
- 当前浏览器所属 session / cgroup 路径

同时把原有“匹配所有 `headless_shell`”的 relevant process 输出改成：

- 当前 browser pid 树
- 少量系统级补充进程摘要

避免把长期残留的无关 Chromium 进程误当成当前 case 证据。

### 2. 采集 browser stdout/stderr 与退出近场信息

对 `browser.process()` 增加：

- stdout/stderr 行缓存
- 退出事件前后的 runtime snapshot
- 子进程退出时的 pid 树快照

这样 browser 被 `SIGKILL`、自崩、或在退出前打印异常时，都能进 case diagnostics。

### 3. 修复系统日志采集兼容性

为当前机器提供兼容回退：

- 优先使用 `journalctl -k --since ...`
- 若不可用，再尝试 `dmesg -T --since ...`
- 若 `dmesg --since` 不支持，则退化到全量 `dmesg -T` 过滤并截尾

目标不是保证完美时间过滤，而是避免“完全没有 kernel evidence”。

### 4. 增加 phase 内周期性资源快照

当前只在 phase 边界采样，离故障点过远。

新增低频周期性快照，覆盖：

- host memory
- node rss
- browser proc status
- 当前 browser pid 树摘要

这样浏览器若在 phase 中途被杀，可以保留更接近故障点的现场。

### 5. 明确标注基础设施故障

在 `run_matrix.mjs` 的 diagnostics 输出中显式区分：

- 业务 verdict fail
- runner / browser infra error
- 次生 `esbuild` service 错误

这一步先增强标注，不改变 matrix 继续执行策略。

## Alternatives Considered

### 直接在 full run 中停止 matrix 后续 case

暂不采用。

这是有效的降噪手段，但它属于执行策略调整，不是纯诊断增强。当前优先目标是拿到首故障根因证据。

### 只增加更多普通日志文本

拒绝。

如果不补 pid/cgroup/kernel/browser stderr 这类结构化证据，只会继续堆叠“症状层”日志。

## Verification Strategy

- `node --check tests/qos_harness/loopback_runner.mjs`
- `node --check tests/qos_harness/run_matrix.mjs`
- `node tests/qos_harness/run_matrix.mjs --cases=T9`
- 选择最小触发链复跑，验证新增 diagnostics 字段会在正常与异常路径中输出

若 full chain 未在本次再次触发首故障，需要明确记录这一点，并说明新增诊断如何覆盖下一次触发。
