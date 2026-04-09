#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== mediasoup-cpp setup ==="

# 1. Check system dependencies
echo "[1/5] Checking system dependencies..."
for cmd in cmake g++ pkg-config curl tar; do
  if ! command -v $cmd &>/dev/null; then
    echo "ERROR: $cmd not found. Install build essentials first."
    echo "  Ubuntu/Debian: apt install build-essential cmake pkg-config libssl-dev zlib1g-dev"
    echo "  CentOS/RHEL:   yum install gcc-c++ cmake openssl-devel zlib-devel"
    exit 1
  fi
done

missing_pkgs=()
for pkg in libavformat libavcodec libavutil hiredis openssl zlib; do
  if ! pkg-config --exists "$pkg" 2>/dev/null; then
    missing_pkgs+=("$pkg")
  fi
done

if [ ${#missing_pkgs[@]} -gt 0 ]; then
  echo "WARNING: missing pkg-config libs: ${missing_pkgs[*]}"
  echo "  Build may fail until required dev packages are installed."
fi
echo "  OK"

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

# 4. Download mediasoup-worker binary
echo "[4/5] Checking mediasoup-worker binary..."
if [ ! -x "mediasoup-worker" ]; then
  echo "  Downloading mediasoup-worker v3.14.16..."
  KERNEL_MAJOR=$(uname -r | cut -d. -f1)
  if [ "$KERNEL_MAJOR" -ge 6 ]; then
    WORKER_URL="https://github.com/versatica/mediasoup/releases/download/3.14.16/mediasoup-worker-3.14.16-linux-x64-kernel6.tgz"
  else
    WORKER_URL="https://github.com/versatica/mediasoup/releases/download/3.14.16/mediasoup-worker-3.14.16-linux-x64-kernel5.tgz"
  fi
  curl -L -o /tmp/mediasoup-worker.tgz "$WORKER_URL"
  tar xzf /tmp/mediasoup-worker.tgz
  chmod +x mediasoup-worker
  rm -f /tmp/mediasoup-worker.tgz
  echo "  Downloaded and extracted"
else
  echo "  Already exists"
fi

# 5. Build
echo "[5/5] Building mediasoup-cpp SFU..."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

echo ""
echo "=== Build complete ==="
echo ""
echo "Binary: $(ls -lh build/mediasoup-sfu | awk '{print $5}')"
echo ""
echo "Run:"
echo "  cd $SCRIPT_DIR"
echo "  ./build/mediasoup-sfu \\"
echo "    --workers=$(nproc) \\"
echo "    --workerBin=$SCRIPT_DIR/mediasoup-worker \\"
echo "    --port=3003 \\"
echo "    --announcedIp=<YOUR_PUBLIC_IP>"
