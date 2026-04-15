cmake_minimum_required(VERSION 3.16)
project(plain-client LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBWS REQUIRED libwebsockets)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(OPUS QUIET opus)

set(JSON_INCLUDE ${CMAKE_SOURCE_DIR}/../third_party/nlohmann_json/include)

add_executable(plain-client main.cpp)

target_include_directories(plain-client PRIVATE
    ${JSON_INCLUDE}
    ${LIBWS_INCLUDE_DIRS}
    ${AVFORMAT_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
)

target_link_libraries(plain-client PRIVATE
    ${LIBWS_LIBRARIES}
    ${AVFORMAT_LIBRARIES}
    ${AVCODEC_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    pthread
)
