## Tasks

1. [completed] 增强 loopback runner 的当前 browser 进程树与 cgroup 诊断
Outcome:
- diagnostics 能输出当前 case browser pid、子进程树、cgroup/session 信息，并避免无关残留 `headless_shell` 混入主证据
Files:
- `tests/qos_harness/loopback_runner.mjs`
Verification:
- `node --check tests/qos_harness/loopback_runner.mjs`

2. [completed] 增强 browser stdout/stderr、退出现场与 kernel 日志兼容采集
Outcome:
- browser stderr/stdout、退出近场快照、兼容 `journalctl`/`dmesg` 回退进入 diagnostics
Files:
- `tests/qos_harness/loopback_runner.mjs`
Verification:
- `node --check tests/qos_harness/loopback_runner.mjs`

3. [completed] 增强 matrix runner 对基础设施故障的诊断标注
Outcome:
- `run_matrix.mjs` 输出能清晰区分首故障与次生错误，打印新增结构化 diagnostics
Files:
- `tests/qos_harness/run_matrix.mjs`
Verification:
- `node --check tests/qos_harness/run_matrix.mjs`

4. [completed] 执行最小复现链并记录结果
Outcome:
- 已验证 `node --check` / `bash -n` 通过，并执行 `QOS_MATRIX_SPEED=0.01 node tests/qos_harness/run_matrix.mjs --cases=T9` smoke；smoke 未触发浏览器基础设施故障，但确认新诊断改动不会破坏 runner 执行
Files:
- `tests/qos_harness/loopback_runner.mjs`
- `tests/qos_harness/run_matrix.mjs`
- `changes/2026-04-19-qos-matrix-crash-diagnostics/tasks.md`
Verification:
- `node tests/qos_harness/run_matrix.mjs --cases=T9`
- 需要时补充最小前置链命令
