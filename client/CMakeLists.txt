cmake_minimum_required(VERSION 3.16)
project(plain-client LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JSON_INCLUDE ${CMAKE_SOURCE_DIR}/../third_party/nlohmann_json/include)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../cmake)
include(ResolveFFmpeg)
resolve_ffmpeg_include_dirs(PLAIN_CLIENT_FFMPEG_INCLUDE_DIRS)
message(STATUS "Using plain-client FFmpeg include dir: ${PLAIN_CLIENT_FFMPEG_INCLUDE_DIRS}")

add_executable(plain-client main.cpp)

target_include_directories(plain-client PRIVATE
    ${JSON_INCLUDE}
    ${PLAIN_CLIENT_FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(plain-client PRIVATE
    avformat avcodec avutil swscale avdevice
    ssl crypto
    pthread
)
