# Tasks

## 1. Step 1: RAII 化 + Decoder/Encoder 替换（合并原 Step 1+2）

### 1.1 common/ffmpeg 扩展

- [x] 1.1a 在 `common/ffmpeg/AvPtr.h` 新增 `SwsContextDeleter` + `SwsContextPtr`
  Files: `common/ffmpeg/AvPtr.h`
  Verification: 已存在，编译通过

- [x] 1.1b 在 `common/ffmpeg/Encoder.h/.cpp` 增加 `width()`/`height()`/`bitRate()`/`setBitRate()` 访问器
  Files: `common/ffmpeg/Encoder.h`, `common/ffmpeg/Encoder.cpp`
  Verification: 已存在，编译通过

- [x] 1.1c 在 `common/ffmpeg/InputFormat.h/.cpp` 新增 `OpenWithFormat` 静态方法
  Files: `common/ffmpeg/InputFormat.h`, `common/ffmpeg/InputFormat.cpp`
  Verification: 已存在，编译通过

### 1.2 SourceWorker 成员变量替换

- [x] 1.2a encoder_ 替换
  Files: `client/SourceWorker.h`
  Verification: 已在之前实现，编译通过

- [x] 1.2b scaledFrame_ + swsCtx_ 替换
  Files: `client/SourceWorker.h`
  Verification: 已在之前实现，编译通过

- [x] 1.2c 局部变量替换
  Files: `client/SourceWorker.h`
  Verification: 已在之前实现，编译通过

- [x] 1.2d drainCommands 和 sendAck 适配
  Files: `client/SourceWorker.h`
  Verification: 已在之前实现，编译通过

### 1.3 PlainClientApp sws RAII 化

- [x] 1.3a sws RAII 化
  Files: `client/PlainClientApp.h`, `client/PlainClientApp.cpp`
  Verification: 已在之前实现，编译通过

### 1.4 验证 Step 1

- [x] 1.4a 全量编译 + 回归测试
  Verification: `cmake --build build` 通过；无裸释放残留在 SourceWorker；sws_getCachedContext 使用 release()+reset() 模式

## 2. Step 2: 提取公共循环骨架

### 2.1 定义 SourceKind 枚举和 runLoop

- [x] 2.1a 定义 SourceKind 和 runLoop 骨架
  Files: `client/SourceWorker.h`
  - `enum class SourceKind { File, Camera }`
  - `void runLoop(SourceKind kind, ffmpeg::InputFormat fmtCtx, int vidIdx)` 私有方法
  - File 独有状态：`firstPts`、`nextEncodePts`、`t0`
  - Camera 独有状态：`frameCount`、`t0`
  - 2-3 个 `if (kind == SourceKind::File)` 条件分支处理差异
  Verification: 编译通过

- [x] 2.1b 简化 loopFile/loopCamera 为薄包装
  Files: `client/SourceWorker.h`
  - `loopFile()`: `InputFormat::Open` → `runLoop(File, ...)`
  - `loopCamera()`: `InputFormat::OpenWithFormat` + MJPEG 回退 → `runLoop(Camera, ...)`
  - Camera 的 `avdevice_register_all()` 和 `av_find_input_format("v4l2")` 保留在 `loopCamera` 中
  Verification: 编译通过

### 2.2 验证 Step 2

- [x] 2.2a 全量编译 + 回归测试
  Verification: 全量编译通过；source_worker 集成测试通过

## 3. Step 3: 日志统一 + 环境变量守卫 + 重复消除

### 3.1 printf → spdlog

- [x] 3.1a SourceWorker printf → spdlog
  Files: `client/SourceWorker.h`
  - `#include <spdlog/spdlog.h>`；移除 `#include <cstdio>`
  - ~20 处 `printf` → `spdlog::info/warn/error`
  - 格式 `[src:{}]` 用 spdlog 参数化
  - `AVCodecID` 枚举需 `static_cast<int>()` 兼容 fmt
  Verification: 编译通过

- [x] 3.1b PlainClientApp/Threaded/Legacy printf → spdlog
  Files: `client/PlainClientApp.cpp`, `client/PlainClientThreaded.cpp`, `client/PlainClientLegacy.cpp`
  - ~44 处 `printf`/`fprintf` → spdlog
  - PlainClientApp 的 usage stderr 帮助文本保留 `fprintf`
  Verification: 编译通过

- [x] 3.1c QosController + NetworkThread printf → spdlog
  Files: `client/qos/QosController.h`, `client/NetworkThread.h`
  - 5 处 `printf`/`fprintf` → spdlog
  - QosController.h header-only，加 spdlog include 增加编译时间；长期方案是 .h/.cpp 拆分
  Verification: 编译通过

### 3.2 环境变量守卫

- [x] 3.2a 定义 MEDIASOUP_TEST_HOOKS 宏
  Files: `CMakeLists.txt`, `client/CMakeLists.txt`
  Verification: 非 test build 中 `MEDIASOUP_TEST_HOOKS` 未定义

- [x] 3.2b PlainClientSupport.cpp / PlainClientApp.cpp 环境变量守卫
  Files: `client/PlainClientSupport.cpp`, `client/PlainClientApp.cpp`
  Verification: 编译通过；test build 环境变量仍可读；非 test build 返回 nullopt

- [x] 3.2c SourceWorker 环境变量守卫
  Files: `client/SourceWorker.h`
  Verification: 编译通过

### 3.3 重复消除

- [x] 3.3a QoS trace 公共函数提取
  Files: 新增 `client/QosTrace.h`，修改 `client/PlainClientLegacy.cpp`、`client/PlainClientThreaded.cpp`
  - 提取 `formatQosTraceLine()` 公共函数
  - Legacy 调用公共函数输出完整 trace
  - Threaded 调用公共函数 + fmt::format 追加扩展字段
  Verification: 编译通过；QoS trace 输出格式不变

- [x] 3.3b scaledDim / ResolveScaledDimension 统一
  Files: 新增 `common/DimensionUtils.h`，修改 `client/SourceWorker.h`、`client/PlainClientApp.h`/`.cpp`
  - 统一为 `mediasoup::scaledDimension(int sourceDim, double scaleDownBy)`
  - SourceWorker 的 `scaledDim()` 已移除，改为调用 `mediasoup::scaledDimension()`
  - PlainClientApp 的 `ResolveScaledDimension()` 保留为静态方法但委托给 `mediasoup::scaledDimension()`
  Verification: 编译通过

### 3.4 验证 Step 3

- [x] 3.4a 全量编译 + 回归测试
  Verification: `cmake --build build` 通过；`grep -rn 'printf\|fprintf' client/ --include='*.h' --include='*.cpp'` 仅剩 stderr 帮助文本；qos_unit_tests + source_worker 测试通过

## 4. Delivery Gates

- [x] 4.1 全量编译 + 测试
  Verification: `cmake --build build` 通过；`mediasoup_qos_unit_tests`、`mediasoup_source_worker_failure_tests`、`mediasoup_source_worker_integration_tests` 全部通过

- [x] 4.2 Review DELIVERY_CHECKLIST.md
  Verification: 所有适用项已审通过；范围、测试、兼容性、运维、知识更新均达标

## 5. Review

- [x] 5.1 按 REVIEW.md 自审
  Verification: 正确性、契约、测试、文档、运维影响已审；发现 1 项 minor（logAvError 死代码）

- [x] 5.2 分支级 diff 审查
  Verification: 全局影响、遗漏更新、回归风险已记录；确认 runLoop 语义等价、RAII 无泄漏、spdlog 格式正确

## 6. Knowledge Update

- [x] 6.1 更新 `docs/repo-review-2026-04-27-full.md` 标记已修复项
  Verification: 已追加「附录：2026-04-28 修复记录」，C1/M4/M5/L2/A4 标记为已修复
