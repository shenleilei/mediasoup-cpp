cmake_minimum_required(VERSION 3.16)
project(plain-client LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(MEDIASOUP_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(JSON_INCLUDE ${MEDIASOUP_REPO_ROOT}/third_party/nlohmann_json/include)

list(APPEND CMAKE_MODULE_PATH ${MEDIASOUP_REPO_ROOT}/cmake)
include(ResolveFFmpeg)
include(MediasoupTargetHelpers)
include(CommonMediaHelpers)
include(CommonFfmpegAdapters)
resolve_ffmpeg_include_dirs(PLAIN_CLIENT_FFMPEG_INCLUDE_DIRS)
message(STATUS "Using plain-client FFmpeg include dir: ${PLAIN_CLIENT_FFMPEG_INCLUDE_DIRS}")

mediasoup_add_interface_target(plain_client_build_common
    INCLUDE_DIRS
        ${MEDIASOUP_REPO_ROOT}/common
        ${MEDIASOUP_REPO_ROOT}/src
        ${JSON_INCLUDE}
        ${PLAIN_CLIENT_FFMPEG_INCLUDE_DIRS}
)
mediasoup_add_interface_target(plain_client_build_ffmpeg_extended
    LINK_LIBRARIES avformat avcodec avutil swscale avdevice
)
mediasoup_add_interface_target(plain_client_build_ssl_crypto
    LINK_LIBRARIES ssl crypto
)
mediasoup_add_interface_target(plain_client_build_pthread
    LINK_LIBRARIES pthread
)

mediasoup_collect_common_media_sources(PLAIN_CLIENT_COMMON_MEDIA_SOURCES ${MEDIASOUP_REPO_ROOT})
mediasoup_collect_common_ffmpeg_sources(PLAIN_CLIENT_COMMON_FFMPEG_SOURCES ${MEDIASOUP_REPO_ROOT})

mediasoup_add_configured_executable(plain-client
    SOURCES
        main.cpp
        PlainClientApp.cpp
        PlainClientLegacy.cpp
        PlainClientThreaded.cpp
        PlainClientSupport.cpp
        WsClient.cpp
        ${PLAIN_CLIENT_COMMON_MEDIA_SOURCES}
        ${PLAIN_CLIENT_COMMON_FFMPEG_SOURCES}
    LINK_LIBRARIES
        plain_client_build_common
        plain_client_build_ffmpeg_extended
        plain_client_build_ssl_crypto
        plain_client_build_pthread
)
