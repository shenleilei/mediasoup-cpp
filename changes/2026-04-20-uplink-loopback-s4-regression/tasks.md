# Tasks

## 1. Reproduce And Isolate

- [x] 1.1 固化当前失败现象
  Files: `/var/log/run_all_tests.log`, `docs/generated/uplink-qos-matrix-report.json`
  Verification: 提取 `R1` / `S4` 的 full-run 与 targeted 结果

- [x] 1.2 比对近期 runner / controller 改动并锁定最小回退点
  Files: `tests/qos_harness/loopback_runner.mjs`, `src/client/lib/qos/controller.js`
  Verification: 锁定 `prio + UDP-only netem` 为 `S4` 回退点；`R1` targeted 单跑未复现

## 2. Fix

- [x] 2.1 实施最小修复，恢复 `S4` 的历史 accepted 行为窗口
  Files: `tests/qos_harness/loopback_runner.mjs`
  Verification: `node tests/qos_harness/run_matrix.mjs --cases=S4`

## 3. Regression Check

- [x] 3.1 确认 `R1,S4` 组合 targeted 通过
  Files: `docs/generated/uplink-qos-matrix-report.targeted.json`
  Verification: `node tests/qos_harness/run_matrix.mjs --cases=R1,S4`

## 4. Knowledge Update

- [x] 4.1 更新必要的 change docs 与结果说明
  Files: `changes/2026-04-20-uplink-loopback-s4-regression/*`, affected `docs/` if needed
  Verification: 文档与最终行为一致
