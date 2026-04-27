# 测试入口点规范 (Test Entrypoints)

## `scripts/run_all_tests.sh`

- `scripts/run_all_tests.sh` 是整个仓库的全量回归测试（full regression）入口。
- 当某个选定的测试组失败时，它**必须 (SHALL)** 继续执行剩余被选中的测试组。
- 如果任何选定的测试组失败，运行结束后它**必须 (SHALL)** 以非零状态码退出。
- 它**必须 (SHALL)** 涵盖 `CMakeLists.txt` 中定义的所有 C++ gtest 目标。
- 它**必须 (SHALL)** 同时涵盖由 `scripts/run_qos_tests.sh` 定义的全部 QoS 回归测试范围。
- 在实际执行测试后，它**必须 (SHALL)** 覆写 `docs/full-regression-test-results.md` 文件。
- 当其选定的组委托给 `scripts/run_qos_tests.sh` 执行时，由该脚本负责的 QoS 相关报告**必须 (SHALL)** 根据该脚本的契约进行刷新。
- 生成的 Markdown 报告**必须 (SHALL)** 包含：
  - 生成时间
  - 本次运行所选定的测试组
  - 整体的通过/失败状态
  - 失败组的摘要信息
  - 每个尝试过的任务的单项状态及耗时
- 使用 `--help` 和 `--list` 参数时，**必须 (SHALL)** 直接退出且不能覆写 `docs/full-regression-test-results.md`。

## `scripts/run_qos_tests.sh`

- `scripts/run_qos_tests.sh` 是 QoS 回归测试入口。
- 当某个选定的任务或测试组失败时，它**必须 (SHALL)** 继续运行剩余选定的组。
- 它的 C++ 单元测试组和 C++ 集成测试组**必须 (SHALL)** 执行专用的 QoS 二进制文件，且不带有单用例白名单过滤（per-case whitelist filters）。
- 它**必须 (SHALL)** 将失败的任务记录到 `tests/qos_harness/artifacts/last-failures.txt` 中。
- 在实际执行之后，它**必须 (SHALL)** 覆写 `docs/downlink-qos-test-results-summary.md`。
- 当矩阵（matrix-style）组运行时，它**必须 (SHALL)** 重新生成在 `README.md` 中记录的对应 QoS Markdown 报告。
- 它的默认 `cpp-client-harness` 测试组**必须 (SHALL)** 包含线程化的纯客户端（threaded plain-client）测试工具回归测试，覆盖：
  - `threaded_generation_switch`
  - `threaded_multi_video_budget`

## `scripts/nightly_full_regression.py`

- `scripts/nightly_full_regression.py run` 是仓库本地的夜间全量回归测试（nightly full-regression）包装脚本。
- 它**必须 (SHALL)** 将实际的测试执行委托给 `scripts/run_all_tests.sh`。
- 它**必须 (SHALL)** 在 `artifacts/nightly-full-regression/` 目录下创建一个带时间戳的运行目录。
- 它**必须 (SHALL)** 仅保留该制品（artifact）根目录下最近 100 个带时间戳的夜间运行目录。
- 它**必须 (SHALL)** 保留每次运行的完整原始日志（raw log）。
- 每次运行后，它**必须 (SHALL)** 刷新配置好的最新日志副本路径。
- 它**必须 (SHALL)** 将配置好的 Markdown 报告附件快照保存到运行目录中。
- 它**必须 (SHALL)** 自动在 git 中记录本次夜间运行新更改的 `docs/` 路径。
- 它**必须 (SHALL)** 排除在运行开始前就已经处于 dirty 状态的 `docs/` 路径。
- 它**必须 (SHALL)** 渲染一封邮件正文，其中包含：
  - 整体状态
  - 任务通过率（如果 `docs/full-regression-test-results.md` 可用）
  - 失败的任务
  - 从原始日志中尽力（best-effort）解析出的失败用例
- 即使测试失败或邮件发送失败，它也**必须 (SHALL)** 保留本地制品（artifacts）。

## 基于日期的报告存档 (Date-Based Report Archives)

- QoS 报告流水线创建的基于日期的报告存档根目录**必须 (SHALL)** 仅保留最近 100 个带时间戳的报告目录。
- 存档保留清理（Archive retention prune）操作**必须 (SHALL)** 仅清理带时间戳的目录，**严禁 (SHALL NOT)** 将辅助符号链接（例如 `latest`）视为清理候选对象。

## `scripts/install_nightly_full_regression_cron.sh`

- `scripts/install_nightly_full_regression_cron.sh` 是用于被托管夜间 cron 任务的仓库本地安装脚本。
- 它**必须 (SHALL)** 安装或刷新单条每日 03:00 执行 `scripts/nightly_full_regression.py run` 的 cron 记录。
- 重复安装**必须 (SHALL)** 是幂等的，并且**严禁 (SHALL NOT)** 产生重复的被托管条目。