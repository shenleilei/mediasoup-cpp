# 依赖与构建环境说明

> 文档性质
>
> 这份文档回答的是：
> - 当前仓库构建 `mediasoup-sfu` / `plain-client` / 测试 / harness 分别依赖什么
> - 哪些依赖是系统安装，哪些是仓库 vendored，哪些是运行时才需要
> - 当前 `CMake` / `setup.sh` 如何解析这些依赖

## 1. 依赖分层

当前仓库的依赖分成四层：

| 层级 | 典型项 | 来源 | 用途 |
|---|---|---|---|
| 系统构建依赖 | `cmake`、`g++`、`pkg-config`、OpenSSL、FFmpeg、hiredis | OS 包管理器 | 编译 SFU、plain-client、测试 |
| 仓库内 vendored 依赖 | `flatbuffers`、`uWebSockets`、`spdlog`、`nlohmann/json`、`ip2region`、`googletest` | `third_party/` | 编译期直接引用或随仓库构建 |
| 运行时外部依赖 | `mediasoup-worker`、Redis、`ip2region.xdb` | 下载 / 本地部署 | 启动 SFU、多节点、Geo 路由 |
| 测试 / harness 依赖 | Node.js、`esbuild`、`puppeteer-core` | `npm` | 浏览器 / QoS harness、矩阵回归 |

## 2. 当前默认构建会编哪些目标

默认的 `./setup.sh` 和常见本地构建会同时构建：

- `build/mediasoup-sfu`
- `client/build/plain-client`
- `build/mediasoup_tests`
- `build/mediasoup_thread_integration_tests`

因此，“当前仓库要能完整构建”这一口径下，下面这些系统库都应视为必需：

- `OpenSSL`
- `zlib`
- `FFmpeg`
  - `libavformat`
  - `libavcodec`
  - `libavutil`
  - `libswscale`
  - `libavdevice`
- `hiredis`
- `pthread`

注意：

- 运行时即使 Redis 不可用，服务端也能退化成 local-only 模式；
- 但**当前默认构建**里 `mediasoup-sfu` 仍直接链接 `hiredis`，所以 `hiredis` 依然是**构建期必需依赖**。

## 3. 系统依赖清单

### 3.1 核心构建工具

必需：

- `cmake >= 3.16`
- `g++ >= 10` 或 `clang >= 12`
- `pkg-config`
- `curl`
- `tar`
- `git`

说明：

- `curl` 和 `tar` 主要给 `setup.sh` 下载并解压 `mediasoup-worker`
- `git` 用于 submodule 初始化或手工拉取 `third_party`

### 3.2 C/C++ 系统库

必需：

- `openssl`
- `zlib`
- `hiredis`
- `libavformat`
- `libavcodec`
- `libavutil`
- `libswscale`
- `libavdevice`

说明：

- `libavdevice` 对当前 threaded `plain-client` 不是“纯可选”库，因为 `client/SourceWorker.h` 已直接包含 `libavdevice/avdevice.h`
- `x264` 不直接由仓库链接，而是通过 FFmpeg 的 H264 encoder 能力间接使用；如果发行版 FFmpeg 缺失对应 encoder，`plain-client` 的重编码路径能力会受限

### 3.3 Debian / Ubuntu 参考安装

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  pkg-config \
  git \
  curl \
  tar \
  libssl-dev \
  zlib1g-dev \
  libhiredis-dev \
  libavformat-dev \
  libavcodec-dev \
  libavutil-dev \
  libswscale-dev \
  libavdevice-dev
```

### 3.4 Fedora / RHEL 参考安装

```bash
sudo dnf install -y \
  gcc-c++ \
  cmake \
  pkgconf-pkg-config \
  git \
  curl \
  tar \
  openssl-devel \
  zlib-devel \
  hiredis-devel \
  ffmpeg-devel
```

如果发行版把 FFmpeg 头文件拆得更细，需确保至少覆盖：

- `libavformat`
- `libavcodec`
- `libavutil`
- `libswscale`
- `libavdevice`

## 4. 仓库内 vendored 依赖

当前由仓库直接管理的依赖包括：

- `third_party/flatbuffers`
- `third_party/uWebSockets`
- `third_party/nlohmann_json`
- `third_party/spdlog`
- `third_party/ip2region`
- `third_party/googletest`

其职责分别是：

| 依赖 | 用途 |
|---|---|
| `flatbuffers` | 生成和使用 `mediasoup-worker` IPC schema 的 C++ 头文件 |
| `uWebSockets` / `uSockets` | HTTP / WebSocket 服务端 |
| `nlohmann/json` | JSON 序列化与控制面协议 |
| `spdlog` | 日志 |
| `ip2region` | IP 地理归属查询 |
| `googletest` | 单元测试与集成测试 |

`setup.sh` 默认会：

- 初始化这些 submodule
- 构建 `flatc`
- 生成 `generated/*_generated.h`
- 构建 vendored `mediasoup-worker`
- 在项目根目录创建 `./mediasoup-worker` 链接到 worker 构建产物

## 5. 运行时依赖

### 5.1 `mediasoup-worker`

`mediasoup-sfu` 不包含 worker 本体，运行时仍需要单独的：

- `./mediasoup-worker`

`setup.sh` 不再下载预编译包，而是从 vendored 源码：

- `src/mediasoup-worker-src`

构建 worker，并在项目根目录创建：

- `./mediasoup-worker -> src/mediasoup-worker-src/worker/out/Release/mediasoup-worker`

当前默认 worker 构建依赖：

- Python 3.8+（或通过 `MEDIASOUP_WORKER_PYTHON` 指定）
- `pip`
- `invoke`（`setup.sh` 会优先从 Aliyun PyPI 镜像安装，缺失时回退到 upstream PyPI）

### 5.2 Redis

Redis 在运行时的定位是：

- 多节点房间归属
- room / node cache 同步
- pub/sub 通知

- 默认运行契约下，Redis 是 readiness 强依赖：

- Redis 不可用时，启动不会成功
- Redis 在运行中失联时，`/readyz` 会失败

如果确实需要本地 isolated 调试，必须显式配置 `redisRequired=false`，而不是依赖隐式降级。

- 无论是否启用 local-only：

- 构建时仍需要 `hiredis`
- 多节点测试和生产路由依赖 Redis

### 5.3 `ip2region.xdb`

Geo 路由相关路径依赖：

- `./ip2region.xdb`

如果根目录没有，服务端会继续尝试：

- `./third_party/ip2region/ip2region.xdb`
- 构建输出目录中的复制版本

## 6. Node / harness 依赖

仓库里有两套 Node 侧依赖：

### 6.1 根目录 `package.json`

当前依赖：

- `esbuild`
- `puppeteer-core`

主要用于：

- 浏览器 bundle
- 某些本地 smoke / harness 路径

安装：

```bash
npm install
```

### 6.2 `tests/qos_harness/package.json`

用于：

- QoS harness
- matrix / capacity / synthetic sweep

安装：

```bash
cd tests/qos_harness
npm install
```

如果只做纯 C++ 构建，不跑浏览器 harness，这两套 Node 依赖都不是强制前置。

## 7. 当前 CMake 的依赖解析规则

### 7.1 FFmpeg

当前 `CMakeLists.txt` 与 `client/CMakeLists.txt` 的解析顺序应理解为：

1. 优先尝试 `pkg-config`
2. 如果 `pkg-config` 没给出完整头文件路径，则尝试显式 include 目录 hint
3. 如果仍找不到 `libavformat/avformat.h`，直接报错

可用的显式 hint 来源包括：

- `FFMPEG_INCLUDE_DIRS` 的 CMake 变量
- `FFMPEG_INCLUDE_DIRS` 环境变量
- `FFMPEG_ROOT/include`
- `FFMPEG_ROOT/include/ffmpeg`
- 常见系统路径

这意味着：

- 推荐优先安装发行版 `*-dev` 包并让 `pkg-config` 工作
- 在非标准安装路径下，可以通过 `FFMPEG_ROOT` 或 `FFMPEG_INCLUDE_DIRS` 显式指定

### 7.2 `hiredis`

当前没有额外的自定义查找逻辑，默认依赖系统链接器能找到：

- `libhiredis`

若链接阶段失败，优先确认：

- `pkg-config --exists hiredis`
- 发行版是否已安装 `libhiredis-dev` / `hiredis-devel`

## 8. `setup.sh` 的职责边界

`setup.sh` 当前负责：

1. 检查基础命令是否存在
2. 检查关键系统库是否能被 `pkg-config` 发现
3. 初始化 `third_party`
4. 构建 `flatc` 并生成 FlatBuffers 头文件
5. 下载 `mediasoup-worker`
6. 构建 SFU、plain-client、测试

它不负责：

- 安装系统包
- 安装 Node 依赖
- 启动 Redis

## 9. 推荐阅读顺序

如果目标是“把环境搭起来并跑通”：

1. 本文档
2. [README.md](../../README.md)
3. [DEVELOPMENT.md](README.md)

如果目标是“理解 Linux plain-client 相关依赖”：

1. 本文档
2. [linux-client-architecture_cn.md](../qos/plain-client/architecture_cn.md)
3. [plain-client-qos-status.md](../qos/plain-client/README.md)
