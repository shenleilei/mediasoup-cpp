# TWCC A/B 最新报告

生成时间：`2026-04-21T20:01:09.632Z`

## 1. 本次运行

- 脚本：`node tests/qos_harness/run_twcc_ab_eval.mjs`
- 重复次数：`1`
- matrix speed：`0.1`
- 场景：`AB-001,AB-002,AB-003,AB-004,AB-005`
- 原始产物目录：`changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T20-01-09.632Z`

## 2. Pairwise 结果

| Pair | Overall | Summary | Markdown | JSON |
|---|---|---|---|---|
| G1 Controller-Only vs G2 Candidate | `FAIL` | G2 Candidate did not meet every configured gate over G1 Controller-Only. | [twcc-ab-g1-vs-g2.md](twcc-ab-g1-vs-g2.md) | [twcc-ab-g1-vs-g2.json](twcc-ab-g1-vs-g2.json) |
| G0 Legacy vs G2 Candidate | `FAIL` | G2 Candidate did not meet every configured gate over G0 Legacy. | [twcc-ab-g0-vs-g2.md](twcc-ab-g0-vs-g2.md) | [twcc-ab-g0-vs-g2.json](twcc-ab-g0-vs-g2.json) |

## 3. 原始产物

- [raw-groups.json](../../changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T20-01-09.632Z/raw-groups.json)
- 原始 trace/log 目录：`changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T20-01-09.632Z/raw`

## 4. 说明

- `G1 vs G2` 用来隔离 transport estimate / TWCC 主路径本身的净收益。
- `G0 vs G2` 用来看新发送路径相对旧路径的整体效果。
- `docs/generated/twcc-ab-report.md` 是最新稳定入口；时间戳目录下保留该次运行的原始产物。
