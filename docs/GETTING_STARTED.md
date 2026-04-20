# 快速上手（首次接手）

这份文档给首次接手仓库的同学一个最短可执行路径：**先装依赖 → 一键构建 → 前台启动 → 做最小验证**。

如果你需要完整依赖背景和分层说明，请看 [dependencies_cn.md](./dependencies_cn.md)。

## 1) 安装系统依赖

### Debian / Ubuntu

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

### Fedora / RHEL

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

## 2) 拉代码并一键构建

```bash
git clone --recursive https://github.com/shenleilei/mediasoup-cpp.git
cd mediasoup-cpp
./setup.sh
```

`setup.sh` 会做这些事：

- 检查 FFmpeg / hiredis / OpenSSL / zlib 等构建依赖
- 初始化 `third_party` 依赖并生成 FlatBuffers 头文件
- 下载 `mediasoup-worker`
- 构建 `mediasoup-sfu`、`plain-client` 和测试二进制

## 3) 前台启动（本地最小模式）

在仓库根目录执行：

```bash
./build/mediasoup-sfu \
  --nodaemon \
  --port=3000 \
  --listenIp=0.0.0.0
```

浏览器访问 `http://<server-ip>:3000`（本机调试可直接用 `http://127.0.0.1:3000`）。

成功标志（最小口径）：

- 页面可以正常打开而不是连接失败/404；
- 至少可以完成进入房间（join）并看到信令请求有正常响应。

> 生产环境请显式设置 `--announcedIp=<public-ip>`，避免依赖自动探测。

## 4) 最小验证

同样在仓库根目录执行：

```bash
./build/mediasoup_tests
./build/mediasoup_qos_unit_tests
```

如果你需要全仓回归入口：

```bash
./scripts/run_all_tests.sh
```

如果你只做 QoS 回归：

```bash
./scripts/run_qos_tests.sh --list
./scripts/run_qos_tests.sh cpp-unit cpp-integration
```

## 5) 常见失败与定位

- `FFmpeg headers not found`  
  没装 FFmpeg 开发包，回到第 1 步安装对应 dev 包（例如 `libavformat-dev/libavcodec-dev/libavutil-dev/libswscale-dev/libavdevice-dev`）。
- `hiredis not found`  
  没装 hiredis 开发包（`libhiredis-dev` / `hiredis-devel`）。
- `required command not found`（来自 `setup.sh`）  
  缺失 `cmake/g++/pkg-config/curl/tar/git`。

更完整排障请看 [troubleshooting_cn.md](./troubleshooting_cn.md)。
