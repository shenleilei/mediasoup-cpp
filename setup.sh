#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== mediasoup-cpp setup ==="

pick_worker_python() {
  local candidates=()

  if [ -n "${MEDIASOUP_WORKER_PYTHON:-}" ]; then
    candidates+=("${MEDIASOUP_WORKER_PYTHON}")
  fi

  candidates+=(python3.12 python3.11 python3.10 python3.9 python3.8 python3)

  for candidate in "${candidates[@]}"; do
    local candidate_path=""

    if [ -x "$candidate" ]; then
      candidate_path="$candidate"
    elif command -v "$candidate" &>/dev/null; then
      candidate_path="$(command -v "$candidate")"
    else
      continue
    fi

    local version
    version="$("$candidate_path" -c 'import sys; print(f"{sys.version_info[0]} {sys.version_info[1]}")')"
    local major minor
    read -r major minor <<< "$version"

    if [ "$major" -gt 3 ] || { [ "$major" -eq 3 ] && [ "$minor" -ge 8 ]; }; then
      printf '%s\n' "$candidate_path"
      return 0
    fi
  done

  return 1
}

install_worker_invoke() {
  local worker_python="$1"
  local invoke_target_dir="$2"
  local pythonpath="${invoke_target_dir}${PYTHONPATH:+:${PYTHONPATH}}"

  if PYTHONPATH="$pythonpath" "$worker_python" -c 'import invoke' &>/dev/null; then
    return 0
  fi

  mkdir -p "$invoke_target_dir"

  if ! "$worker_python" -m pip install --upgrade --no-user --target "$invoke_target_dir" invoke -i https://mirrors.aliyun.com/pypi/simple; then
    echo "  Aliyun PyPI mirror missing invoke or failed; falling back to upstream PyPI"
    "$worker_python" -m pip install --upgrade --no-user --target "$invoke_target_dir" invoke -i https://pypi.org/simple
  fi
}

# 1. Check system dependencies
echo "[1/5] Checking system dependencies..."
for cmd in cmake g++ pkg-config git; do
  if ! command -v $cmd &>/dev/null; then
    echo "ERROR: $cmd not found. Install build essentials first."
    echo "  Ubuntu/Debian: apt install build-essential cmake pkg-config git libssl-dev zlib1g-dev"
    echo "  Fedora/RHEL:   dnf install gcc-c++ cmake pkgconf-pkg-config git openssl-devel zlib-devel"
    exit 1
  fi
done

if ! WORKER_PYTHON="$(pick_worker_python)"; then
  echo "ERROR: no Python 3.8+ interpreter found for building mediasoup-worker."
  echo "  Set MEDIASOUP_WORKER_PYTHON=/path/to/python3.8+ if needed."
  exit 1
fi

if ! "$WORKER_PYTHON" -m pip --version &>/dev/null; then
  echo "ERROR: pip is not available for $WORKER_PYTHON."
  echo "  Install python3-pip for the selected Python interpreter."
  exit 1
fi

missing_pkgs=()
required_pkg_config_libs=(
  libavformat
  libavcodec
  libavutil
  libswscale
  libavdevice
  hiredis
  openssl
  zlib
)

for pkg in "${required_pkg_config_libs[@]}"; do
  if ! pkg-config --exists "$pkg" 2>/dev/null; then
    missing_pkgs+=("$pkg")
  fi
done

if [ ${#missing_pkgs[@]} -gt 0 ]; then
  echo "ERROR: missing pkg-config libs: ${missing_pkgs[*]}"
  echo "  Required for current default build (SFU + plain-client + tests)."
  echo "  Ubuntu/Debian:"
  echo "    apt install libssl-dev zlib1g-dev libhiredis-dev \\"
  echo "      libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libavdevice-dev"
  echo "  Fedora/RHEL:"
  echo "    dnf install openssl-devel zlib-devel hiredis-devel ffmpeg-devel"
  exit 1
fi
if ! command -v node &>/dev/null; then
  echo "NOTE: node not found — browser / QoS harness will not be available"
fi
echo "  OK"
echo "  Worker build Python: $WORKER_PYTHON"

# 2. Init git submodules (if cloned from git)
echo "[2/5] Setting up third-party dependencies..."
if [ -f ".gitmodules" ]; then
  git submodule update --init --recursive
else
  # Manual clone if not using submodules
  mkdir -p third_party
  [ -d "third_party/flatbuffers/.git" ] || git clone --depth 1 https://github.com/google/flatbuffers.git third_party/flatbuffers
  [ -d "third_party/uWebSockets/.git" ] || git clone --depth 1 --recurse-submodules https://github.com/uNetworking/uWebSockets.git third_party/uWebSockets
  [ -d "third_party/nlohmann_json/.git" ] || git clone --depth 1 https://github.com/nlohmann/json.git third_party/nlohmann_json
  [ -d "third_party/spdlog/.git" ] || git clone --depth 1 https://github.com/gabime/spdlog.git third_party/spdlog
fi

if [ ! -f "third_party/ip2region/binding/c/xdb_api.h" ] || \
   [ ! -f "third_party/ip2region/binding/c/xdb_searcher.c" ] || \
   [ ! -f "third_party/ip2region/binding/c/xdb_util.c" ]; then
  echo "ERROR: bundled ip2region C binding files are missing."
  echo "  Expected under third_party/ip2region/binding/c/"
  exit 1
fi
echo "  OK"

# 3. Build flatc and generate C++ headers from FBS schemas
echo "[3/5] Generating FlatBuffers C++ headers..."
if [ ! -f "third_party/flatbuffers/build/flatc" ]; then
  echo "  Building flatc compiler..."
  cmake -B third_party/flatbuffers/build -S third_party/flatbuffers \
    -DCMAKE_BUILD_TYPE=Release -DFLATBUFFERS_BUILD_TESTS=OFF -DFLATBUFFERS_BUILD_FLATHASH=OFF
  cmake --build third_party/flatbuffers/build -j$(nproc) --target flatc
fi
FLATC="third_party/flatbuffers/build/flatc"
mkdir -p generated
cd fbs
for f in *.fbs; do
  ../$FLATC --cpp -o ../generated/ --gen-object-api --scoped-enums "$f"
done
cd ..
echo "  Generated $(ls generated/*_generated.h | wc -l) header files"

# 4. Build vendored mediasoup-worker and expose it at ./mediasoup-worker
echo "[4/5] Building vendored mediasoup-worker..."
WORKER_INVOKE_DIR="$SCRIPT_DIR/src/mediasoup-worker-src/worker/out/pip_setup_invoke"
install_worker_invoke "$WORKER_PYTHON" "$WORKER_INVOKE_DIR"
PYTHONPATH="$WORKER_INVOKE_DIR${PYTHONPATH:+:${PYTHONPATH}}" \
PYTHON="$WORKER_PYTHON" \
  "$WORKER_PYTHON" -m invoke -r src/mediasoup-worker-src/worker mediasoup-worker
rm -f mediasoup-worker
ln -s src/mediasoup-worker-src/worker/out/Release/mediasoup-worker mediasoup-worker
echo "  Built $(readlink -f mediasoup-worker)"

# 5. Build
echo "[5/5] Building mediasoup-cpp SFU + plain-client + tests..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Build plain-client (threaded multi-source publisher)
cmake -B client/build -S client -DCMAKE_BUILD_TYPE=Release
cmake --build client/build -j$(nproc)

echo ""
echo "=== Build complete ==="
echo ""
echo "SFU binary:    $(ls -lh build/mediasoup-sfu 2>/dev/null | awk '{print $5, $NF}')"
echo "Plain client:  $(ls -lh client/build/plain-client 2>/dev/null | awk '{print $5, $NF}')"
echo "Unit tests:    $(ls -lh build/mediasoup_tests 2>/dev/null | awk '{print $5, $NF}')"
echo "Thread tests:  $(ls -lh build/mediasoup_thread_integration_tests 2>/dev/null | awk '{print $5, $NF}')"
echo ""
echo "Run SFU:"
echo "  cd $SCRIPT_DIR"
echo "  ./build/mediasoup-sfu \\"
echo "    --workers=$(nproc) \\"
echo "    --workerBin=$SCRIPT_DIR/mediasoup-worker \\"
echo "    --port=3003 \\"
echo "    --announcedIp=<YOUR_PUBLIC_IP>"
echo ""
echo "Run plain-client (single track, legacy mode):"
echo "  ./client/build/plain-client SERVER_IP PORT ROOM PEER file.mp4"
echo ""
echo "Run plain-client (multi-camera, threaded mode):"
echo "  PLAIN_CLIENT_THREADED=1 \\"
echo "  PLAIN_CLIENT_VIDEO_TRACK_COUNT=2 \\"
echo "  PLAIN_CLIENT_VIDEO_SOURCES=/dev/video0,/dev/video2 \\"
echo "  ./client/build/plain-client SERVER_IP PORT ROOM PEER dummy.mp4"
echo ""
echo "Run tests:"
echo "  cd $SCRIPT_DIR && ./build/mediasoup_tests"
echo "  cd $SCRIPT_DIR && ./build/mediasoup_thread_integration_tests"
echo ""
echo "Optional browser / harness dependencies:"
echo "  npm install"
echo "  (cd tests/qos_harness && npm install)"
