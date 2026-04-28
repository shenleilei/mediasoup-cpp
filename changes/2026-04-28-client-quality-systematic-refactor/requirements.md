# Requirements

## Summary

系统性重构 `client/` 模块，消除裸 FFmpeg 指针、printf 日志、环境变量注入生产路径、重复代码等质量问题，使 client/ 与项目 `common/ffmpeg/` RAII 层和 `src/` 的 spdlog 日志标准对齐。

## Business Goal

当前 `client/` 是代码库质量最差的区域：SourceWorker 完全绕过 RAII 层、50+ 处 printf 替代 spdlog、`QOS_TEST_*` 环境变量无守卫可注入生产路径。这些问题是 review 报告中 C1（avcodec_open2 返回值未检查）的根因，也阻碍了后续功能开发的可维护性。修复后可降低运行时崩溃风险、提升可调试性、堵住安全漏洞。

## In Scope

- SourceWorker RAII 化：裸指针 → `common/ffmpeg/` RAII 类型
- SourceWorker 用 `common/ffmpeg::Decoder` / `Encoder` 替换手写编解码逻辑
- SourceWorker 提取 loopFile/loopCamera 公共循环骨架（enum + 条件分支，不用回调）
- `Encoder` 类增加 `width()`/`height()`/`bitRate()`/`setBitRate()` 访问器
- `InputFormat` 增加 `OpenWithFormat` 静态方法（支持 camera 自定义 format/opts）
- `QOS_TEST_*` 环境变量加 `MEDIASOUP_TEST_HOOKS` 宏守卫
- `client/` 全部 printf → spdlog
- QoS trace 提取公共函数（PlainClientLegacy / PlainClientThreaded 重复）
- `scaledDim` / `ResolveScaledDimension` 统一
- PlainClientApp sws 裸指针 RAII 化

## Out Of Scope

- `src/` 目录变更（已用 spdlog 和 RAII）
- `common/ffmpeg/` 内部重构（仅新增类型和方法，不改现有接口）
- 新增功能或行为变更（两个 UB→异常 改善除外）
- 测试文件的重构
- QoS header-only → .cpp 拆分（独立变更）
- SourceWorker .h/.cpp 拆分（可在 Step 2 提取循环后自然发生，不作为独立目标）

## User Stories

### Story 1

作为开发者，我希望 SourceWorker 使用 `common/ffmpeg/` RAII 封装，以便消除手动释放错误和 avcodec_open2 返回值未检查的风险。

### Story 2

作为运维人员，我希望 `QOS_TEST_*` 环境变量在生产构建中不可用，以防止误配置导致 QoS 决策被篡改。

### Story 3

作为开发者，我希望 `client/` 使用 spdlog 统一日志，以便与 `src/` 保持一致、支持日志级别控制和结构化输出。

### Story 4

作为开发者，我希望 loopFile/loopCamera 共享循环骨架，以便减少 170 行重复代码、降低维护成本。

## Acceptance Criteria

### AC1: SourceWorker RAII 化 + Decoder/Encoder 替换

系统 SHALL 使用 `Encoder`、`Decoder`、`FramePtr`、`PacketPtr`、`SwsContextPtr`、`InputFormat` 管理所有 FFmpeg 资源，不再有手动 `avcodec_free_context` / `av_frame_free` / `av_packet_free` / `sws_freeContext` / `avformat_close_input` 调用。

#### Scenario: 编码器创建失败

- WHEN `Encoder::Create` 中 `avcodec_open2` 返回负值
- THEN 抛出 `std::runtime_error`，调用层 try/catch 捕获，RAII 析构自动清理已分配的上下文

#### Scenario: 解码器创建失败

- WHEN `Decoder::OpenFromParameters` 抛异常
- THEN 异常在 loopFile/loopCamera 层被 catch，资源自动释放

#### Scenario: 循环任意点退出

- WHEN `loopFile`/`loopCamera` 在任意点因错误退出
- THEN 所有 RAII 对象析构，无手动释放代码

#### Scenario: bit_rate 原地更新

- WHEN `drainCommands` 收到仅 bit_rate 变化的 SetEncodingParameters
- THEN 调用 `encoder_.setBitRate()` 更新，不重建编码器

### AC2: Encoder 访问器

系统 SHALL 在 `Encoder` 类提供 `width()`/`height()`/`bitRate()`/`setBitRate()` 方法，避免外部通过 `.get()` 穿透到 `AVCodecContext*`。

### AC3: InputFormat 扩展

系统 SHALL 提供 `InputFormat::OpenWithFormat(path, fmt, opts)` 静态方法，支持 V4L2 camera 的自定义 format 和 options 打开。

### AC4: QOS_TEST_* 环境变量守卫

系统 SHALL 在 `MEDIASOUP_TEST_HOOKS` 未定义时使 `QOS_TEST_*` 环境变量读取返回 `nullptr`。

#### Scenario: Production build 读取 QOS_TEST_MATRIX_PROFILE

- WHEN 代码编译时 `MEDIASOUP_TEST_HOOKS` 未定义
- THEN `loadMatrixTestProfileFromEnv()` 返回 `std::nullopt`，即使环境变量已设置

#### Scenario: Test build 读取 QOS_TEST_MATRIX_PROFILE

- WHEN 代码编译时 `MEDIASOUP_TEST_HOOKS` 已定义（`BUILD_TESTING=ON`）
- THEN 环境变量正常读取

### AC5: client/ printf → spdlog

系统 SHALL 将 `client/` 下所有 printf/fprintf 替换为 spdlog 调用。

#### Scenario: 全量替换

- WHEN 搜索 `client/` 下 printf/fprintf
- THEN 结果仅剩 `PlainClientApp` 的 usage 提示（stderr 帮助文本）

### AC6: 循环骨架提取

系统 SHALL 将 loopFile/loopCamera 重复逻辑提取为 `runLoop(SourceKind, InputFormat, vidIdx)` 公共方法，使用 enum + 条件分支处理 File/Camera 差异。

#### Scenario: File source

- WHEN 调用 `loopFile()`
- THEN 行为与重构前一致

#### Scenario: Camera source

- WHEN 调用 `loopCamera()`
- THEN 行为与重构前一致

## Non-Functional Requirements

- Performance: RAII 析构开销为零（编译器内联 unique_ptr 析构）；enum 条件分支零堆分配
- Reliability: 每步重构后 test_synthetic_sweep 回归通过
- Security: `QOS_TEST_*` 在非 test build 不可触发
- Compatibility: 行为变更仅限 UB→异常（严格改善），其余零变更
- Observability: spdlog 支持日志级别控制，替换后可按需开关

## Edge Cases

- `initEncoder` 原地更新 bit_rate（不重建编码器）的行为必须保留
- `sws_getCachedContext` 返回同一指针的复用语义需精确处理：必须 `release()` + `reset()` 模式，禁止直接 `reset(sws_getCachedContext(swsCtx_.get(), ...))`
- `av_frame_get_buffer` / `av_frame_make_writable` 失败从静默 UB 变为抛异常：
  - `initEncoder` 中 `FrameGetBuffer` 失败 → 整个初始化失败
  - `encodeAndEnqueue` 中 `FrameMakeWritable` 失败 → 丢弃当前帧继续
- Camera 的 MJPEG 回退逻辑需在 `InputFormat::OpenWithFormat` 两次调用中正确表达
- `AVDictionary* opts` 在 camera 打开失败时需 `av_dict_free`，`InputFormat::OpenWithFormat` 失败时 opts 已被 FFmpeg 消费或需手动释放
- `Encoder` 包装后 `drainCommands` 和 `sendAck` 需通过访问器访问 width/height，不穿透到 `.get()`
- Camera loop 中 `rtpTs` 变量（L497）是死代码，重构时删除

## Open Questions

- `InputFormat::OpenWithFormat` 失败时 `AVDictionary**` 的生命周期：FFmpeg 的 `avformat_open_input` 在失败时不释放 opts，调用者需负责 `av_dict_free`。需在 `OpenWithFormat` 文档中明确此约定。
- QosController.h header-only 加 spdlog include 增加编译时间，长期方案是 .h/.cpp 拆分。是否在本次变更中一并拆分？（建议不在本次 scope，记录为 follow-up）
