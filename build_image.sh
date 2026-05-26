#!/usr/bin/env bash
set -euo pipefail

info() {
  >&2 printf "[\033[34m\033[1mINFO\033[0m] %s\n" "$*"
}

warn() {
  >&2 printf "[\033[33m\033[1mWARN\033[0m] %s\n" "$*"
}

ok() {
  >&2 printf "[\033[32m\033[1m OK \033[0m] %s\n" "$*"
}

usage() {
  cat <<'EOF'
Usage:
  ./build_image.sh [--push] [--tag TAG] [--repository REPOSITORY] [--latest]

Build the mediasoup-cpp SFU Docker image. By default this only builds the local
image. Use --push to push it to a registry.

Environment overrides:
  IMAGE_NAME          Local image name. Default: mediasoup-cpp
  IMAGE_VARIANT       Local image variant/tag alias. Default: sfu
  IMAGE_TAG           Version tag. Default: current timestamp, YYYY_mm_dd_HHMM
  DOCKER_REPOSITORY   Remote repository. Default:
                      harbor-volc.zelostech.com.cn:5443/arch/mediasoup-cpp
  DOCKER_USERNAME     Registry username used when --push is set.
  DOCKER_PASSWORD     Registry password/token used when --push is set.
  DOCKER_REGISTRY     Registry host for docker login. Parsed from repository by default.
  APT_MIRROR          Ubuntu apt mirror build arg. Default:
                      http://mirrors.aliyun.com/ubuntu

Examples:
  ./build_image.sh
  ./build_image.sh --push
  DOCKER_USERNAME=arch DOCKER_PASSWORD=*** ./build_image.sh --push --latest
EOF
}

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    >&2 printf "ERROR: required command not found: %s\n" "$1"
    exit 1
  fi
}

ensure_build_context_deps() {
  local missing=0
  local required_files=(
    third_party/flatbuffers/CMakeLists.txt
    third_party/uWebSockets/src/App.h
    third_party/nlohmann_json/include/nlohmann/json.hpp
    third_party/spdlog/CMakeLists.txt
    third_party/ip2region/binding/c/xdb_api.h
    third_party/ip2region/ip2region.xdb
  )

  for file in "${required_files[@]}"; do
    if [[ ! -f "$file" ]]; then
      missing=1
      break
    fi
  done

  if [[ "$missing" -eq 0 ]]; then
    return
  fi

  info "initializing git submodules required by Docker build context"
  git submodule update --init --recursive

  for file in "${required_files[@]}"; do
    if [[ ! -f "$file" ]]; then
      >&2 printf "ERROR: required build context file is missing after submodule update: %s\n" "$file"
      exit 1
    fi
  done
}

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$repo_root"

image_name="${IMAGE_NAME:-mediasoup-cpp}"
image_variant="${IMAGE_VARIANT:-sfu}"
image_tag="${IMAGE_TAG:-$(date +'%Y_%m_%d_%H%M')}"
dockerfile="${DOCKERFILE:-Dockerfile}"
repository="${DOCKER_REPOSITORY:-harbor-volc.zelostech.com.cn:5443/arch/${image_name}}"
apt_mirror="${APT_MIRROR:-http://mirrors.aliyun.com/ubuntu}"
push_image=0
push_latest=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --push)
      push_image=1
      ;;
    --latest)
      push_latest=1
      ;;
    --tag)
      image_tag="${2:?missing value for --tag}"
      shift
      ;;
    --repository)
      repository="${2:?missing value for --repository}"
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      >&2 printf "ERROR: unknown argument: %s\n" "$1"
      usage >&2
      exit 1
      ;;
  esac
  shift
done

require_cmd docker

if [[ ! -f "$dockerfile" ]]; then
  >&2 printf "ERROR: Dockerfile not found: %s\n" "$dockerfile"
  exit 1
fi

ensure_build_context_deps

local_tag="${image_name}:${image_variant}"
version_tag="${repository}:${image_tag}"
docker_build_cmd=(docker build)

if docker buildx version >/dev/null 2>&1; then
  docker_build_cmd+=(--progress=plain)
fi

info "building Docker image ${local_tag}"
${docker_build_cmd[@]} \
  -f "$dockerfile" \
  --build-arg "APT_MIRROR=${apt_mirror}" \
  -t "$local_tag" \
  .

info "built local image ${local_tag}"
info "version tag will be ${version_tag}"

if [[ "$push_image" -ne 1 ]]; then
  ok "build complete; skip push because --push was not provided"
  exit 0
fi

registry="${DOCKER_REGISTRY:-${repository%%/*}}"
if [[ -n "${DOCKER_USERNAME:-}" && -n "${DOCKER_PASSWORD:-}" ]]; then
  info "logging in to ${registry} as ${DOCKER_USERNAME}"
  printf "%s" "$DOCKER_PASSWORD" | docker login "$registry" \
    --username="$DOCKER_USERNAME" \
    --password-stdin
else
  warn "DOCKER_USERNAME/DOCKER_PASSWORD not set; assuming docker is already logged in to ${registry}"
fi

docker tag "$local_tag" "$version_tag"
docker push "$version_tag"
info "pushed ${version_tag}"

if [[ "$push_latest" -eq 1 ]]; then
  latest_tag="${repository}:latest"
  docker tag "$local_tag" "$latest_tag"
  docker push "$latest_tag"
  info "pushed ${latest_tag}"
fi

if [[ -n "${DOCKER_USERNAME:-}" && -n "${DOCKER_PASSWORD:-}" ]]; then
  docker logout "$registry" >/dev/null 2>&1 || true
fi

ok "build and push completed"
