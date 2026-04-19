## Symptom

`scripts/run_all_tests.sh` 触发的 full QoS regression 中，`matrix` 组在 `T9` 处偶发执行错误：

- `Attempted to use detached Frame ...`
- 伴随 `browser_disconnected`
- `browser_process_exit` 记录为 `signal=SIGKILL`

随后 `T10` 到 `S4` 继续报：

- `The service is no longer running`

这会把单个基础设施故障放大成多条 matrix error。

## Reproduction

当前已观察到的稳定复现入口：

- `scripts/run_all_tests.sh`
- `scripts/run_qos_tests.sh all`

当前已观察到的非复现入口：

- `node tests/qos_harness/run_matrix.mjs --cases=T9`

单独跑 `T9` 时，case 会按业务判定失败或通过，但不会稳定复现浏览器 `SIGKILL`。

## Observed Behavior

- runner 日志可以看见 Puppeteer 页面的 detached frame 症状
- runner 日志可以看见浏览器进程退出信号是 `SIGKILL`
- 后续 case 继续运行并产出 `esbuild` 服务已失效的次生错误
- 当次诊断里，内核日志采集命令 `dmesg -T --since ...` 在当前机器不兼容，导致缺少首故障系统证据
- 现有 relevant process 输出会混入系统中其他长期残留的 `headless_shell` 进程，弱化当前 case 的可读性

## Expected Behavior

- 当 browser matrix runner 出现基础设施级崩溃时，日志必须能区分：
  - 当前 case 自身业务失败
  - 浏览器进程被外部终止
  - 浏览器自身 crash
  - `esbuild` 或其他共享辅助进程的次生故障
- 诊断输出必须包含足够的系统、进程、cgroup、browser stderr/stdout 与时间窗证据，能在一次 full run 后判断首故障来源
- 诊断输出必须聚焦当前 case 创建的浏览器进程树，避免长期残留 `headless_shell` 干扰分析

## Suspected Scope

- `tests/qos_harness/loopback_runner.mjs`
- `tests/qos_harness/run_matrix.mjs`

可能涉及：

- browser 子进程 stdout/stderr 采集
- 当前浏览器 pid 树与 cgroup 信息采集
- `dmesg`/`journalctl` 兼容回退
- 长 phase 中的周期性资源快照
- 基础设施错误与次生错误的标注

## Known Non-Affected Behavior

- 单独执行 `T9` 不会稳定触发浏览器 `SIGKILL`
- 当前 matrix 业务断言逻辑仍可正常评估 isolated `T9`
- downlink priority 失败是独立的业务行为问题，不属于本 bugfix 范围

## Acceptance Criteria

- full run 或最小复现链再次触发 browser matrix 基础设施故障时，日志中能直接看出首个失效组件
- 诊断输出能明确关联到当前 case 的 browser pid、pid 树、cgroup/session 信息
- 内核日志采集在当前机器上不再因 `dmesg --since` 不兼容而完全失效
- browser 进程如果打印 stderr/stdout、退出前后状态、OOM 相关线索，这些信息会进入 case diagnostics
- relevant process 输出不再把无关长期残留浏览器实例误当成当前 case 证据

## Regression Expectations

- `run_matrix.mjs --cases=T9` 的既有业务判定语义不变
- downlink/browser/node/cpp harness 的既有执行入口仍可正常运行
- 新增诊断逻辑在成功 case 上不会改变 PASS/FAIL 业务判定
