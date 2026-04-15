cmake_minimum_required(VERSION 3.16)
project(plain-client LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JSON_INCLUDE ${CMAKE_SOURCE_DIR}/../third_party/nlohmann_json/include)

add_executable(plain-client main.cpp)

target_include_directories(plain-client PRIVATE
    ${JSON_INCLUDE}
    /usr/include/ffmpeg
)

target_link_libraries(plain-client PRIVATE
    avformat avcodec avutil
    ssl crypto
    pthread
)
