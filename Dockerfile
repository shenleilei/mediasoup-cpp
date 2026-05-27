FROM ubuntu:20.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ARG APT_MIRROR=http://mirrors.aliyun.com/ubuntu

RUN sed -i "s|http://archive.ubuntu.com/ubuntu|${APT_MIRROR}|g; s|http://security.ubuntu.com/ubuntu|${APT_MIRROR}|g" /etc/apt/sources.list \
  && apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    git \
    libavcodec-dev \
    libavdevice-dev \
    libavformat-dev \
    libavutil-dev \
    libhiredis-dev \
    libssl-dev \
    libswscale-dev \
    pkg-config \
    tar \
    zlib1g-dev \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY . .

RUN if [ -d .git ]; then \
    git submodule update --init --recursive; \
  else \
    test -f third_party/flatbuffers/CMakeLists.txt; \
    test -f third_party/uWebSockets/src/App.h; \
    test -f third_party/nlohmann_json/include/nlohmann/json.hpp; \
    test -f third_party/spdlog/CMakeLists.txt; \
    test -f third_party/ip2region/binding/c/xdb_api.h; \
    test -f third_party/ip2region/ip2region.xdb; \
  fi \
  && cmake -B third_party/flatbuffers/build -S third_party/flatbuffers \
    -DCMAKE_BUILD_TYPE=Release \
    -DFLATBUFFERS_BUILD_TESTS=OFF \
    -DFLATBUFFERS_BUILD_FLATHASH=OFF \
  && cmake --build third_party/flatbuffers/build --target flatc -j"$(nproc)" \
  && mkdir -p generated \
  && cd fbs \
  && ../third_party/flatbuffers/build/flatc --cpp -o ../generated/ --gen-object-api --scoped-enums *.fbs \
  && cd .. \
  && test -x mediasoup-worker \
  && cmake -S . -B build-docker \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=OFF \
  && cmake --build build-docker --target mediasoup-sfu -j"$(nproc)" \
  && install -Dm755 build-docker/mediasoup-sfu /opt/mediasoup-cpp/mediasoup-sfu \
  && install -Dm755 mediasoup-worker /opt/mediasoup-cpp/mediasoup-worker \
  && install -Dm644 third_party/ip2region/ip2region.xdb /opt/mediasoup-cpp/third_party/ip2region/ip2region.xdb \
  && cp -a public /opt/mediasoup-cpp/public

FROM ubuntu:20.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
ARG APT_MIRROR=http://mirrors.aliyun.com/ubuntu

RUN sed -i "s|http://archive.ubuntu.com/ubuntu|${APT_MIRROR}|g; s|http://security.ubuntu.com/ubuntu|${APT_MIRROR}|g" /etc/apt/sources.list \
  && apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    libavcodec58 \
    libavdevice58 \
    libavformat58 \
    libavutil56 \
    libhiredis0.14 \
    libssl1.1 \
    libswscale5 \
    zlib1g \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/mediasoup-cpp

COPY --from=builder /opt/mediasoup-cpp/ ./
COPY docker/entrypoint.sh /usr/local/bin/mediasoup-sfu-entrypoint

RUN chmod +x /usr/local/bin/mediasoup-sfu-entrypoint \
  && mkdir -p /var/log/mediasoup-cpp

EXPOSE 1770/tcp
EXPOSE 8000-8002/udp

STOPSIGNAL SIGTERM

ENV MEDIASOUP_PORT=1770 \
    MEDIASOUP_LISTEN_IP=0.0.0.0 \
    MEDIASOUP_ANNOUNCED_IP= \
    MEDIASOUP_RTC_MIN_PORT=8000 \
    MEDIASOUP_RTC_MAX_PORT=8002 \
    MEDIASOUP_REDIS_REQUIRED=0 \
    MEDIASOUP_LOG_DIR=/var/log/mediasoup-cpp

ENTRYPOINT ["mediasoup-sfu-entrypoint"]
