cmake_minimum_required(VERSION 3.16)
project(mediasoup-cpp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Third-party: FlatBuffers (header-only usage) ──
set(FLATBUFFERS_INCLUDE ${CMAKE_SOURCE_DIR}/third_party/flatbuffers/include)

# ── Third-party: nlohmann/json (header-only) ──
set(JSON_INCLUDE ${CMAKE_SOURCE_DIR}/third_party/nlohmann_json/include)

# ── Third-party: spdlog ──
add_subdirectory(third_party/spdlog EXCLUDE_FROM_ALL)

# ── Third-party: uSockets (build as static lib) ──
set(USOCKETS_DIR ${CMAKE_SOURCE_DIR}/third_party/uWebSockets/uSockets)
file(GLOB USOCKETS_SRC ${USOCKETS_DIR}/src/*.c ${USOCKETS_DIR}/src/crypto/*.c ${USOCKETS_DIR}/src/crypto/*.cpp ${USOCKETS_DIR}/src/eventing/*.c)
add_library(uSockets STATIC ${USOCKETS_SRC})
target_include_directories(uSockets PUBLIC ${USOCKETS_DIR}/src)
target_compile_definitions(uSockets PUBLIC LIBUS_USE_OPENSSL)
target_link_libraries(uSockets PUBLIC ssl crypto)

# ── Generated FlatBuffers headers ──
set(GENERATED_INCLUDE ${CMAKE_SOURCE_DIR}/generated)

# ── uWebSockets (header-only) ──
set(UWS_INCLUDE ${CMAKE_SOURCE_DIR}/third_party/uWebSockets/src)

# ── Main library ──
add_library(mediasoup_lib STATIC
    src/Channel.cpp
    src/Worker.cpp
    src/Transport.cpp
    src/WebRtcTransport.cpp
    src/PipeTransport.cpp
    src/Router.cpp
    src/Producer.cpp
    src/Consumer.cpp
)

target_include_directories(mediasoup_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${FLATBUFFERS_INCLUDE}
    ${JSON_INCLUDE}
    ${GENERATED_INCLUDE}
    ${UWS_INCLUDE}
    ${USOCKETS_DIR}/src
)

target_link_libraries(mediasoup_lib PUBLIC spdlog::spdlog uSockets pthread)

# ── Main executable ──
add_executable(mediasoup-sfu
    src/main.cpp
    src/SignalingServer.cpp
)

target_include_directories(mediasoup-sfu PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${FLATBUFFERS_INCLUDE}
    ${JSON_INCLUDE}
    ${GENERATED_INCLUDE}
    ${UWS_INCLUDE}
    ${USOCKETS_DIR}/src
)

target_link_libraries(mediasoup-sfu PRIVATE mediasoup_lib z)
