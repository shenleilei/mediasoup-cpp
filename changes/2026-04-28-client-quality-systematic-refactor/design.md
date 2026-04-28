# Design

## Context

`client/` 目录与项目其余部分存在三层质量断层：

1. **FFmpeg RAII 断层**：`common/ffmpeg/` 提供了 `AvPtr.h`（CodecContextPtr/FramePtr/PacketPtr）、`Decoder`、`Encoder`、`InputFormat`、`BitstreamFilter` 五个 RAII 封装。SourceWorker 完全不用，直接操作 12 处裸指针。PlainClientApp 有一处 `sws_freeContext`。
2. **日志断层**：`src/` 统一用 spdlog，`client/` 有 50+ 处 printf/fprintf。
3. **安全断层**：`QOS_TEST_*` 环境变量无编译期守卫，生产环境可触发，最高可注入任意 JSON 到 QoS 决策和 stats 上报。

代码结构层面，SourceWorker 的 `loopFile()` 和 `loopCamera()` 有 90% 重复（~170 行），PlainClientLegacy/PlainClientThreaded 的 QoS trace 格式化也高度重复。

## Proposed Approach

分三步递进式重构，每步可独立提交和验证：

### Step 1: RAII 化 + Decoder/Encoder 替换（合并原 Step 1+2）

**目标**：SourceWorker 的所有 FFmpeg 操作走项目统一封装，消除手动释放和 avcodec_open2 返回值未检查的根因。

#### 1a. 扩展 InputFormat 支持自定义打开参数

当前 `InputFormat::Open()` 只接受 path，不支持 V4L2 camera 所需的自定义 format 和 options。新增静态方法：

```cpp
// InputFormat.h
static InputFormat OpenWithFormat(const std::string& path,
    const AVInputFormat* fmt, AVDictionary** opts);
```

实现复用现有 `Close()`/析构逻辑。Camera 的 MJPEG 回退用两次 `OpenWithFormat` 调用表达，不引入裸指针。

不新增 `FormatContextPtr`——`InputFormat` 已是完整的 RAII 封装，重复封装违反 DRY。

#### 1b. 新增 SwsContextPtr 到 AvPtr.h

```cpp
struct SwsContextDeleter {
    void operator()(SwsContext* ctx) const { if (ctx) sws_freeContext(ctx); }
};
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;
```

`sws_getCachedContext` 的 RAII 语义需精确处理——该函数在参数不同时内部释放旧指针并创建新指针，参数相同时返回原指针。正确模式：

```cpp
// encodeAndEnqueue 中的替换
SwsContext* raw = swsCtx_.release();  // 放弃所有权
SwsContext* result = sws_getCachedContext(raw,
    vframe->width, vframe->height, (AVPixelFormat)vframe->format,
    encoder_.width(), encoder_.height(), AV_PIX_FMT_YUV420P,
    SWS_BILINEAR, nullptr, nullptr, nullptr);
swsCtx_.reset(result);  // 接管返回值（可能是原指针或新指针）
```

**错误写法**（会导致 double-free）：`swsCtx_.reset(sws_getCachedContext(swsCtx_.get(), ...))`

#### 1c. SourceWorker 成员变量替换

| 原类型 | 新类型 | 说明 |
|--------|--------|------|
| `AVCodecContext* encoder_` | `ffmpeg::Encoder encoder_` | 使用 Encoder RAII 封装 |
| `AVFrame* scaledFrame_` | `ffmpeg::FramePtr scaledFrame_` | 使用 FramePtr RAII |
| `SwsContext* swsCtx_` | `ffmpeg::SwsContextPtr swsCtx_` | 新增 RAII 类型 |

局部变量同理：`vdec` → `ffmpeg::Decoder`，`vframe` → `ffmpeg::FramePtr`，`pkt` → `ffmpeg::PacketPtr`，`fmtCtx` → `ffmpeg::InputFormat`。

#### 1d. Encoder 类增加访问器

当前 `drainCommands` 和 `sendAck` 频繁访问 `encoder_->width` / `encoder_->height` / `encoder_->bit_rate`。如果 `encoder_` 变为 `Encoder` 类型，外部需 `.get()` 穿透到 `AVCodecContext*`，违反封装。

在 `common/ffmpeg/Encoder.h` 增加访问器：

```cpp
int width() const { return context_ ? context_->width : 0; }
int height() const { return context_ ? context_->height : 0; }
int64_t bitRate() const { return context_ ? context_->bit_rate : 0; }
```

`drainCommands` 的 bit_rate 原地更新路径改为：

```cpp
// 仅 bit_rate 变化时，不重建编码器
encoder_.setBitRate(le.bitrateBps);  // 新增方法，内部设 bit_rate/rc_max_rate/rc_buffer_size
```

#### 1e. initDecoder/initEncoder 替换

- `initDecoder()` 删除，替换为 `ffmpeg::Decoder::OpenFromParameters(par)`
  - 异常在调用层 try/catch，转为 `running_ = false` + 日志
- `initEncoder()` 删除，替换为 `ffmpeg::Encoder::Create(AV_CODEC_ID_H264, configureFn)`
  - `configureFn` lambda 设置 width/height/fps/bitrate/preset/tune/profile
  - `av_opt_set` 调用放在 `configureFn` 内（`avcodec_open2` 之前）
- `encodeAndEnqueue` 中 `avcodec_send_frame` / `avcodec_receive_packet` → `encoder_.SendFrame()` / `encoder_.ReceivePacket()`

#### 1f. RAII 引入的隐式行为变更

以下操作从"静默 UB"变为"抛异常"：

| 操作 | 当前行为 | RAII 后行为 | 处理 |
|------|----------|-------------|------|
| `av_frame_get_buffer` 失败（initEncoder） | 写入未分配帧（UB） | `FrameGetBuffer` 抛异常 | 接受——帧分配失败不可能恢复，异常终止优于 UB |
| `av_frame_make_writable` 失败（encodeAndEnqueue） | 写入只读帧（UB） | `FrameMakeWritable` 抛异常 | try/catch 在 encodeAndEnqueue 层，丢弃当前帧继续 |

这不是纯重构，但"UB→异常"是严格改善。在 tasks 中标注此行为变更。

#### 1g. 删除所有手动释放代码

loopFile/loopCamera 尾部 6 行 × 2 = 12 行手动释放全部删除，由 RAII 析构自动处理。

#### 1h. PlainClientApp sws RAII 化

`track.swsCtx` → `ffmpeg::SwsContextPtr swsCtx`，删除手动 `sws_freeContext`。

### Step 2: 提取公共循环骨架

**目标**：消除 loopFile/loopCamera 170 行重复。

#### 2a. 设计选择：enum + 条件分支，不用回调

原方案用 `std::function` 回调（`computePts`/`pace`）抽象 File 和 Camera 差异，但回调携带可变状态（`firstPts`、`nextEncodePts`、`t0`），且 File 的 pacing 是 per-packet、FPS 过滤是 per-frame，两者交错在解码循环内无法拆为独立回调。

改用 enum + 条件分支：

```cpp
enum class SourceKind { File, Camera };

void runLoop(SourceKind kind, ffmpeg::InputFormat fmtCtx, int vidIdx) {
    // 公共：创建 Decoder、Encoder、vframe、pkt
    // File 独有状态：firstPts, nextEncodePts, t0
    // Camera 独有状态：frameCount, t0
    while (running_.load() && fmtCtx.ReadPacket(pkt.get())) {
        if (pkt->stream_index != vidIdx) { av_packet_unref(pkt.get()); continue; }
        drainCommands();
        if (!running_.load()) break;

        if (kind == SourceKind::File) {
            // pacing: sleep_until
            // FPS 过滤: nextEncodePts
        }
        // Camera: no pacing, wall-clock PTS

        if (paused_) { av_packet_unref(pkt.get()); continue; }

        // 公共：decode + encode
    }
    // RAII 析构，无手动清理
}
```

File/Camera 差异用 2-3 个 `if (kind == ...)` 分支处理，增加约 10 行但零堆分配、零间接调用、易调试。

#### 2b. 输入打开逻辑

```cpp
void loopFile() {
    auto fmt = ffmpeg::InputFormat::Open(cfg_.inputPath);
    runLoop(SourceKind::File, std::move(fmt), findVideoStream(fmt));
}

void loopCamera() {
    avdevice_register_all();
    auto* v4l2Fmt = av_find_input_format("v4l2");
    // 第一次尝试 MJPEG
    AVDictionary* opts = buildCameraOpts(/* mjpeg= */ true);
    try {
        auto fmt = ffmpeg::InputFormat::OpenWithFormat(cfg_.inputPath, v4l2Fmt, &opts);
        av_dict_free(&opts);
        runLoop(SourceKind::Camera, std::move(fmt), findVideoStream(fmt));
    } catch (...) {
        // 回退：不指定 MJPEG
        av_dict_free(&opts);
        opts = buildCameraOpts(/* mjpeg= */ false);
        auto fmt = ffmpeg::InputFormat::OpenWithFormat(cfg_.inputPath, v4l2Fmt, &opts);
        av_dict_free(&opts);
        runLoop(SourceKind::Camera, std::move(fmt), findVideoStream(fmt));
    }
}
```

#### 2c. 额外清理：Camera loop 死代码

`loopCamera` L497 计算了 `rtpTs` 但从未使用（`encodeAndEnqueue` 内部从 `ptsSec` 重新计算）。重构时删除此变量。

### Step 3: 日志统一 + 环境变量守卫 + 重复消除

**目标**：client/ 对齐 spdlog，堵住 QOS_TEST_* 注入。

#### 3a. SourceWorker printf → spdlog

~20 处 `printf` → `spdlog::info/warn/error`，格式 `[src:{}]` 用 spdlog 参数化。

#### 3b. PlainClientApp/Threaded/Legacy printf → spdlog

~44 处 `printf`/`fprintf` → spdlog。PlainClientApp 的 usage stderr 帮助文本保留 `fprintf`。

#### 3c. QosController + NetworkThread printf → spdlog

5 处 `printf`/`fprintf` → spdlog。

**注意**：`QosController.h` 是 header-only。加 `#include <spdlog/spdlog.h>` 会增加所有包含方的编译时间。这是已知代价，长期方案是 QoS header-only → .cpp 拆分（不在本次 scope）。

#### 3d. QOS_TEST_* 环境变量守卫

不用 `NDEBUG`（语义是"disable assert"，不是"production build"）。定义 `MEDIASOUP_TEST_HOOKS` 宏，仅在 test build 中定义。

各调用点统一改为：

```cpp
#ifdef MEDIASOUP_TEST_HOOKS
    const char* raw = std::getenv("QOS_TEST_MATRIX_PROFILE");
#else
    const char* raw = nullptr;
#endif
```

受影响文件：
- `client/PlainClientSupport.cpp`：`QOS_TEST_MATRIX_PROFILE`、`QOS_TEST_CLIENT_STATS_PAYLOADS`、`QOS_TEST_SELF_REQUESTS`
- `client/PlainClientApp.cpp`：`QOS_TEST_MATRIX_LOCAL_ONLY`、`QOS_TEST_FORCE_STALE_TRACK_INDEX`
- `client/SourceWorker.h`：`QOS_TEST_REJECT_FIRST_SET_ENCODING_TRACK_INDEX`

CMake 变更：root `CMakeLists.txt` 和 standalone `client/CMakeLists.txt` 均在 test build 时通过 `target_compile_definitions` 注入 `MEDIASOUP_TEST_HOOKS`。

`src/qos/SubscriberBudgetAllocator.cpp` 的 `MEDIASOUP_QOS_BASE_BITRATE_BPS` 等属于运维调参，不加守卫。

#### 3e. QoS trace 公共函数提取

新增 `client/QosTrace.h`，提取 `formatQosTraceLine()` 函数。PlainClientLegacy/Threaded 调用公共函数。

#### 3f. scaledDim / ResolveScaledDimension 统一

新增 `common/DimensionUtils.h`，统一为 `scaledDimension(int sourceDim, double scaleDownBy)`。SourceWorker 和 PlainClientApp 调用公共版本。

## Alternatives Considered

### A1: 不重构，只修 C1（avcodec_open2 返回值）

- 理由：最小变更
- 拒绝：不解决根因（裸指针模式），后续同类 bug 会再出现

### A2: SourceWorker 整体重写为 .h/.cpp 拆分

- 理由：header-only 542 行不规范
- 拒绝：当前 step 已含拆分（Step 2 提取循环后自然需要 .cpp），但不作为独立 step，避免 scope 膨胀

### A3: 一步到位全部重构

- 理由：减少中间状态
- 拒绝：变更量太大无法单次验证，分三步每步可独立回归

### A4: 新增 FormatContextPtr 而非扩展 InputFormat

- 理由：不修改 common/ 接口
- 拒绝：与已有 InputFormat 重复封装，违反 DRY；扩展 InputFormat 是更小变更（加一个静态方法）

### A5: SourceInput 回调抽象

- 理由：消除循环内的条件分支
- 拒绝：回调携带可变状态、堆分配、难调试；File 的 pacing + FPS 过滤交错在解码循环内无法拆为独立回调；enum + 条件分支更直观

### A6: NDEBUG 作为测试守卫

- 理由：标准宏，无需自定义
- 拒绝：语义是"disable assert"而非"production build"，许多生产构建不带 -DNDEBUG；MEDIASOUP_TEST_HOOKS 语义更精确且由 CMake 显式控制

## Modules And Responsibilities

- `common/ffmpeg/InputFormat.h/.cpp`：新增 `OpenWithFormat` 静态方法
- `common/ffmpeg/AvPtr.h`：新增 `SwsContextPtr` 类型别名
- `common/ffmpeg/Encoder.h/.cpp`：增加 `width()`/`height()`/`bitRate()`/`setBitRate()` 访问器
- `client/SourceWorker.h`：RAII 化 + Decoder/Encoder 替换 + 循环骨架提取 + spdlog
- `client/PlainClientApp.h/.cpp`：sws RAII 化 + printf → spdlog
- `client/PlainClientThreaded.cpp`：printf → spdlog + QoS trace 共用
- `client/PlainClientLegacy.cpp`：printf → spdlog + QoS trace 共用
- `client/qos/QosController.h`：printf → spdlog
- `client/NetworkThread.h`：fprintf → spdlog
- `client/PlainClientSupport.cpp`：QOS_TEST_* → MEDIASOUP_TEST_HOOKS 守卫
- `client/QosTrace.h`（新增）：QoS trace 格式化公共函数
- `common/DimensionUtils.h`（新增）：scaledDimension 公共函数
- `CMakeLists.txt` / `client/CMakeLists.txt`：test build 时定义 `MEDIASOUP_TEST_HOOKS`

## Data And State

- SourceWorker 成员变量变更：
  - `AVCodecContext* encoder_` → `ffmpeg::Encoder encoder_`
  - `AVFrame* scaledFrame_` → `ffmpeg::FramePtr scaledFrame_`
  - `SwsContext* swsCtx_` → `ffmpeg::SwsContextPtr swsCtx_`
- 无新增全局状态
- 无 schema 变更

## Interfaces

- `common/ffmpeg/InputFormat.h`：新增 `OpenWithFormat()` 静态方法（纯新增，不影响现有 `Open()`）
- `common/ffmpeg/AvPtr.h`：新增 `SwsContextPtr`（纯新增类型别名）
- `common/ffmpeg/Encoder.h`：新增 `width()`/`height()`/`bitRate()`/`setBitRate()` 访问器（纯新增，不影响现有接口）
- SourceWorker 公共接口不变（start/stop/Config/queue 指针）
- 日志输出格式变更：`[src:0]` 前缀 → spdlog 参数化

## Failure Handling

- `Decoder::OpenFromParameters` 抛异常 → loopFile/loopCamera 中 try/catch，`running_ = false` + spdlog::error
- `Encoder::Create` 抛异常 → drainCommands 中 try/catch，sendAck(applied=false)
- `FrameGetBuffer`/`FrameMakeWritable` 抛异常 → initEncoder 层 try/catch（整个初始化失败）；encodeAndEnqueue 层 try/catch（丢弃当前帧继续）
- SwsContextPtr 的 `sws_getCachedContext`：必须先 `release()` 再 `reset()`，禁止直接 `reset(sws_getCachedContext(swsCtx_.get(), ...))`

## Security Considerations

- `QOS_TEST_*` 环境变量在 `MEDIASOUP_TEST_HOOKS` 未定义时不可触发，消除生产环境注入风险
- `MEDIASOUP_QOS_BASE_BITRATE_BPS` 等属于运维调参，不加守卫
- spdlog 不影响安全，仅替换日志基础设施

## Testing Strategy

- **Unit tests**：现有 `test_source_worker_failure.cpp` 验证 SourceWorker 错误路径
- **Integration tests**：`test_synthetic_sweep` 每步重构后回归验证
- **Regression**：`test_common_ffmpeg.cpp` 验证 common/ffmpeg RAII 层不受影响
- **Manual**：检查非 `BUILD_TESTING` 构建下 `QOS_TEST_MATRIX_PROFILE` 不生效

## Observability

- spdlog 替换后可按级别控制日志输出
- QoS trace 输出格式不变（仅底层从 printf → spdlog::info）

## Rollout Notes

- 纯重构 + 两个严格改善的行为变更（UB→异常）
- 无需 feature flag
- 回滚：git revert 对应 commit 即可
- 依赖：spdlog 已是项目依赖，无需新增
