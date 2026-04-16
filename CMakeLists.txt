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
    src/GeoDbResolver.cpp
    src/Worker.cpp
    src/Transport.cpp
    src/WebRtcTransport.cpp
    src/Router.cpp
    src/Producer.cpp
    src/Consumer.cpp
    src/qos/QosSnapshot.cpp
    src/qos/QosValidator.cpp
    src/qos/QosRegistry.cpp
    src/qos/QosAggregator.cpp
    src/qos/QosRoomAggregator.cpp
    src/qos/QosOverride.cpp
    src/qos/DownlinkQosRegistry.cpp
    src/qos/DownlinkAllocator.cpp
    src/qos/SubscriberQosController.cpp
    src/qos/DownlinkHealthMonitor.cpp
    src/qos/SubscriberBudgetAllocator.cpp
    src/qos/ProducerDemandAggregator.cpp
    src/qos/PublisherSupplyController.cpp
    src/qos/RoomDownlinkPlanner.cpp
)

target_include_directories(mediasoup_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${FLATBUFFERS_INCLUDE}
    ${JSON_INCLUDE}
    ${GENERATED_INCLUDE}
    ${UWS_INCLUDE}
    ${USOCKETS_DIR}/src
    /usr/include/ffmpeg
)

target_link_libraries(mediasoup_lib PUBLIC spdlog::spdlog uSockets pthread avformat avcodec avutil)

# ── Third-party: ip2region (C library) ──
set(IP2REGION_DIR ${CMAKE_SOURCE_DIR}/third_party/ip2region/binding/c)
add_library(ip2region STATIC ${IP2REGION_DIR}/xdb_searcher.c ${IP2REGION_DIR}/xdb_util.c)
target_include_directories(ip2region PUBLIC ${IP2REGION_DIR})

# Copy ip2region.xdb to build output directory
configure_file(
    ${CMAKE_SOURCE_DIR}/third_party/ip2region/ip2region.xdb
    ${CMAKE_BINARY_DIR}/ip2region.xdb COPYONLY)

# ── Main executable ──
add_executable(mediasoup-sfu
    src/main.cpp
    src/SignalingServer.cpp
    src/RoomService.cpp
)

target_include_directories(mediasoup-sfu PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${FLATBUFFERS_INCLUDE}
    ${JSON_INCLUDE}
    ${GENERATED_INCLUDE}
    ${UWS_INCLUDE}
    ${USOCKETS_DIR}/src
    /usr/include/ffmpeg
    ${IP2REGION_DIR}
)

target_link_libraries(mediasoup-sfu PRIVATE mediasoup_lib ip2region z hiredis avformat avcodec avutil)

# ── Tests ──
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL)
    enable_testing()

    set(TEST_SOURCES
        tests/test_client_qos.cpp
        tests/test_downlink_allocator.cpp
        tests/test_downlink_health.cpp
        tests/test_downlink_v2.cpp
        tests/test_geo_db_resolver.cpp
        tests/test_geo_router.cpp
        tests/test_multinode.cpp
        tests/test_ortc.cpp
        tests/test_qos_accuracy.cpp
        tests/test_qos_aggregator.cpp
        tests/test_qos_override.cpp
        tests/test_qos_protocol.cpp
        tests/test_qos_registry.cpp
        tests/test_qos_room_aggregator.cpp
        tests/test_qos_unit.cpp
        tests/test_qos_validator.cpp
        tests/test_request_timeout.cpp
        tests/test_review_fixes.cpp
        tests/test_room.cpp
        tests/test_rtp_types.cpp
        tests/test_stability.cpp
        tests/test_supported_capabilities.cpp
        tests/test_utils.cpp
        tests/test_worker_queue.cpp
    )
    add_executable(mediasoup_tests ${TEST_SOURCES})
    target_include_directories(mediasoup_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${FLATBUFFERS_INCLUDE}
        ${JSON_INCLUDE}
        ${GENERATED_INCLUDE}
        ${IP2REGION_DIR}
    )
    target_link_libraries(mediasoup_tests PRIVATE gtest gtest_main mediasoup_lib ip2region)
    add_test(NAME mediasoup_tests COMMAND mediasoup_tests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # Integration (black-box) tests - requires built mediasoup-sfu + mediasoup-worker
    add_executable(mediasoup_integration_tests tests/test_integration.cpp)
    target_include_directories(mediasoup_integration_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}/src
        ${JSON_INCLUDE}
        /usr/include/ffmpeg
    )
    target_link_libraries(mediasoup_integration_tests PRIVATE gtest gtest_main spdlog::spdlog pthread avformat avcodec avutil)
    add_test(NAME mediasoup_integration_tests COMMAND mediasoup_integration_tests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # E2E publish/subscribe stress test
    add_executable(mediasoup_e2e_tests tests/test_e2e_pubsub.cpp)
    target_include_directories(mediasoup_e2e_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}/src
        ${JSON_INCLUDE}
    )
    target_link_libraries(mediasoup_e2e_tests PRIVATE gtest gtest_main pthread)
    add_test(NAME mediasoup_e2e_tests COMMAND mediasoup_e2e_tests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # QoS integration tests
    add_executable(mediasoup_qos_integration_tests tests/test_qos_integration.cpp)
    target_include_directories(mediasoup_qos_integration_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}/src
        ${JSON_INCLUDE}
    )
    target_link_libraries(mediasoup_qos_integration_tests PRIVATE gtest gtest_main pthread)
    add_test(NAME mediasoup_qos_integration_tests COMMAND mediasoup_qos_integration_tests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # Multi-SFU topology test
    add_executable(mediasoup_topology_tests tests/test_multi_sfu_topology.cpp)
    target_include_directories(mediasoup_topology_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}/src
        ${JSON_INCLUDE}
    )
    target_link_libraries(mediasoup_topology_tests PRIVATE gtest gtest_main pthread hiredis)
    add_test(NAME mediasoup_topology_tests COMMAND mediasoup_topology_tests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # Stability integration tests
    add_executable(mediasoup_stability_integration_tests tests/test_stability_integration.cpp)
    target_include_directories(mediasoup_stability_integration_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}/src
        ${JSON_INCLUDE}
    )
    target_link_libraries(mediasoup_stability_integration_tests PRIVATE gtest gtest_main pthread)
    add_test(NAME mediasoup_stability_integration_tests COMMAND mediasoup_stability_integration_tests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # Multi-node integration tests (requires Redis + mediasoup-worker)
    add_executable(mediasoup_multinode_tests tests/test_multinode_integration.cpp)
    target_include_directories(mediasoup_multinode_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}/src
        ${JSON_INCLUDE}
    )
    target_link_libraries(mediasoup_multinode_tests PRIVATE gtest gtest_main pthread hiredis)
    add_test(NAME mediasoup_multinode_tests COMMAND mediasoup_multinode_tests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # Review fixes integration tests (2026-04-08)
    add_executable(mediasoup_review_fix_tests tests/test_review_fixes_integration.cpp)
    target_include_directories(mediasoup_review_fix_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}/src
        ${JSON_INCLUDE}
    )
    target_link_libraries(mediasoup_review_fix_tests PRIVATE gtest gtest_main pthread hiredis)
    add_test(NAME mediasoup_review_fix_tests COMMAND mediasoup_review_fix_tests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # Worker load benchmark (not a test, standalone binary)
    add_executable(mediasoup_bench tests/bench_worker_load.cpp)
    target_include_directories(mediasoup_bench PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${FLATBUFFERS_INCLUDE}
        ${JSON_INCLUDE}
        ${GENERATED_INCLUDE}
    )
    target_link_libraries(mediasoup_bench PRIVATE mediasoup_lib pthread)

    # QoS accuracy tests (requires mediasoup-worker binary)
    add_executable(mediasoup_qos_accuracy_tests tests/test_qos_accuracy.cpp)
    target_include_directories(mediasoup_qos_accuracy_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${FLATBUFFERS_INCLUDE}
        ${JSON_INCLUDE}
        ${GENERATED_INCLUDE}
    )
    target_link_libraries(mediasoup_qos_accuracy_tests PRIVATE gtest gtest_main mediasoup_lib pthread)
    add_test(NAME mediasoup_qos_accuracy_tests COMMAND mediasoup_qos_accuracy_tests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # QoS recording accuracy tests (requires mediasoup-worker + ffmpeg)
    add_executable(mediasoup_qos_recording_accuracy_tests tests/test_qos_recording_accuracy.cpp)
    target_include_directories(mediasoup_qos_recording_accuracy_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${FLATBUFFERS_INCLUDE}
        ${JSON_INCLUDE}
        ${GENERATED_INCLUDE}
        /usr/include/ffmpeg
    )
    target_link_libraries(mediasoup_qos_recording_accuracy_tests PRIVATE
        gtest gtest_main mediasoup_lib pthread avformat avcodec avutil)
    add_test(NAME mediasoup_qos_recording_accuracy_tests COMMAND mediasoup_qos_recording_accuracy_tests
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()
