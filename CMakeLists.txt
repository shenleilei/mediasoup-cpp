cmake_minimum_required(VERSION 3.16)
project(mediasoup-cpp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(ResolveFFmpeg)
include(MediasoupTargetHelpers)
include(CommonMediaHelpers)
include(CommonFfmpegAdapters)
resolve_ffmpeg_include_dirs(MEDIASOUP_FFMPEG_INCLUDE_DIRS)
message(STATUS "Using FFmpeg include dir: ${MEDIASOUP_FFMPEG_INCLUDE_DIRS}")

set(FLATBUFFERS_INCLUDE ${CMAKE_SOURCE_DIR}/third_party/flatbuffers/include)
set(JSON_INCLUDE ${CMAKE_SOURCE_DIR}/third_party/nlohmann_json/include)
set(GENERATED_INCLUDE ${CMAKE_SOURCE_DIR}/generated)
set(UWS_INCLUDE ${CMAKE_SOURCE_DIR}/third_party/uWebSockets/src)
set(USOCKETS_DIR ${CMAKE_SOURCE_DIR}/third_party/uWebSockets/uSockets)
set(IP2REGION_DIR ${CMAKE_SOURCE_DIR}/third_party/ip2region/binding/c)

set(MEDIASOUP_BUILD_COMMON_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/src
    ${FLATBUFFERS_INCLUDE}
    ${JSON_INCLUDE}
    ${GENERATED_INCLUDE}
    ${UWS_INCLUDE}
    ${USOCKETS_DIR}/src
    ${MEDIASOUP_FFMPEG_INCLUDE_DIRS}
)

add_subdirectory(third_party/spdlog EXCLUDE_FROM_ALL)

mediasoup_add_interface_target(mediasoup_build_ssl_crypto
    LINK_LIBRARIES ssl crypto
)
mediasoup_add_interface_target(mediasoup_build_pthread
    LINK_LIBRARIES pthread
)
mediasoup_add_interface_target(mediasoup_build_ffmpeg_core
    LINK_LIBRARIES avformat avcodec avutil
)
mediasoup_add_interface_target(mediasoup_build_ffmpeg_extended
    LINK_LIBRARIES mediasoup_build_ffmpeg_core swscale avdevice
)
mediasoup_add_interface_target(mediasoup_build_common
    INCLUDE_DIRS ${MEDIASOUP_BUILD_COMMON_INCLUDE_DIRS}
)
mediasoup_add_interface_target(mediasoup_build_test_support
    INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/tests
    LINK_LIBRARIES mediasoup_build_common
)

file(GLOB USOCKETS_SRC
    ${USOCKETS_DIR}/src/*.c
    ${USOCKETS_DIR}/src/crypto/*.c
    ${USOCKETS_DIR}/src/crypto/*.cpp
    ${USOCKETS_DIR}/src/eventing/*.c
)
add_library(uSockets STATIC ${USOCKETS_SRC})
target_include_directories(uSockets PUBLIC ${USOCKETS_DIR}/src)
target_compile_definitions(uSockets PUBLIC LIBUS_USE_OPENSSL)
target_link_libraries(uSockets PUBLIC mediasoup_build_ssl_crypto)

set(MEDIASOUP_LIBRARY_SOURCES
    src/Channel.cpp
    src/GeoDbResolver.cpp
    src/Recorder.cpp
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
add_library(mediasoup_lib STATIC ${MEDIASOUP_LIBRARY_SOURCES})

target_link_libraries(mediasoup_lib PUBLIC
    mediasoup_build_common
    mediasoup_build_pthread
    mediasoup_build_ffmpeg_core
    spdlog::spdlog
    uSockets
)

mediasoup_collect_common_media_sources(MEDIASOUP_COMMON_MEDIA_SOURCES ${CMAKE_SOURCE_DIR})
add_library(mediasoup_common_media STATIC ${MEDIASOUP_COMMON_MEDIA_SOURCES})
target_link_libraries(mediasoup_common_media PUBLIC mediasoup_build_common)

mediasoup_collect_common_ffmpeg_sources(MEDIASOUP_COMMON_FFMPEG_SOURCES ${CMAKE_SOURCE_DIR})
add_library(mediasoup_common_ffmpeg STATIC ${MEDIASOUP_COMMON_FFMPEG_SOURCES})
target_link_libraries(mediasoup_common_ffmpeg PUBLIC mediasoup_build_common mediasoup_build_ffmpeg_extended)

target_link_libraries(mediasoup_lib PUBLIC mediasoup_common_media mediasoup_common_ffmpeg)

target_link_libraries(mediasoup_build_test_support INTERFACE mediasoup_common_media mediasoup_common_ffmpeg)

add_library(ip2region STATIC ${IP2REGION_DIR}/xdb_searcher.c ${IP2REGION_DIR}/xdb_util.c)
target_include_directories(ip2region PUBLIC ${IP2REGION_DIR})

configure_file(
    ${CMAKE_SOURCE_DIR}/third_party/ip2region/ip2region.xdb
    ${CMAKE_BINARY_DIR}/ip2region.xdb COPYONLY)

mediasoup_add_configured_executable(mediasoup-sfu
    SOURCES
        src/main.cpp
        src/MainBootstrap.cpp
        src/RoomRegistry.cpp
        src/RuntimeDaemon.cpp
        src/SignalingServer.cpp
        src/SignalingServerHttp.cpp
        src/SignalingServerRuntime.cpp
        src/SignalingServerWs.cpp
        src/RoomService.cpp
        src/RoomServiceDownlink.cpp
        src/RoomServiceLifecycle.cpp
        src/RoomServiceMedia.cpp
        src/RoomServiceQos.cpp
        src/RoomServiceRegistryView.cpp
        src/RoomServiceStats.cpp
        src/RoomRecordingHelpers.cpp
        src/WorkerThread.cpp
    LINK_LIBRARIES
        mediasoup_lib
        mediasoup_common_media
        mediasoup_common_ffmpeg
        ip2region
        z
        hiredis
)

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    add_subdirectory(third_party/googletest EXCLUDE_FROM_ALL)
    enable_testing()

    set(MEDIASOUP_QOS_UNIT_TEST_SOURCES
        tests/test_client_qos.cpp
        tests/test_downlink_allocator.cpp
        tests/test_downlink_health.cpp
        tests/test_downlink_v2.cpp
        tests/test_qos_aggregator.cpp
        tests/test_qos_override.cpp
        tests/test_qos_protocol.cpp
        tests/test_qos_registry.cpp
        tests/test_qos_room_aggregator.cpp
        tests/test_qos_unit.cpp
        tests/test_qos_validator.cpp
        tests/test_thread_model.cpp
    )

    set(MEDIASOUP_UNIT_TEST_SOURCES
        client/WsClient.cpp
        tests/test_geo_db_resolver.cpp
        tests/test_geo_router.cpp
        tests/test_common_media.cpp
        tests/test_multinode.cpp
        tests/test_ortc.cpp
        tests/test_request_timeout.cpp
        tests/test_review_fixes.cpp
        tests/test_room.cpp
        tests/test_rtp_types.cpp
        tests/test_stability.cpp
        tests/test_supported_capabilities.cpp
        tests/test_utils.cpp
        tests/test_ws_client.cpp
        tests/test_worker_queue.cpp
    )

    mediasoup_add_gtest_target(mediasoup_tests
        SOURCES ${MEDIASOUP_UNIT_TEST_SOURCES}
        LINK_LIBRARIES
            mediasoup_build_test_support
            mediasoup_lib
            ip2region
    )

    mediasoup_add_gtest_target(mediasoup_qos_unit_tests
        SOURCES ${MEDIASOUP_QOS_UNIT_TEST_SOURCES}
        LINK_LIBRARIES
            mediasoup_build_test_support
            mediasoup_lib
            ip2region
    )

    mediasoup_add_gtest_target(mediasoup_integration_tests
        SOURCES tests/test_integration.cpp
        LINK_LIBRARIES
            mediasoup_build_test_support
            mediasoup_lib
            mediasoup_build_pthread
            mediasoup_build_ffmpeg_core
            spdlog::spdlog
    )

    mediasoup_add_gtest_target(mediasoup_e2e_tests
        SOURCES tests/test_e2e_pubsub.cpp
        LINK_LIBRARIES
            mediasoup_build_test_support
            mediasoup_build_pthread
    )

    mediasoup_add_gtest_target(mediasoup_qos_integration_tests
        SOURCES tests/test_qos_integration.cpp
        LINK_LIBRARIES
            mediasoup_build_test_support
            mediasoup_build_pthread
    )

    mediasoup_add_gtest_target(mediasoup_topology_tests
        SOURCES tests/test_multi_sfu_topology.cpp
        LINK_LIBRARIES
            mediasoup_build_test_support
            mediasoup_build_pthread
            hiredis
    )

    mediasoup_add_gtest_target(mediasoup_stability_integration_tests
        SOURCES tests/test_stability_integration.cpp
        LINK_LIBRARIES
            mediasoup_build_test_support
            mediasoup_build_pthread
    )

    mediasoup_add_gtest_target(mediasoup_multinode_tests
        SOURCES tests/test_multinode_integration.cpp
        LINK_LIBRARIES
            mediasoup_build_test_support
            mediasoup_build_pthread
            hiredis
    )

    mediasoup_add_gtest_target(mediasoup_review_fix_tests
        SOURCES tests/test_review_fixes_integration.cpp
        LINK_LIBRARIES
            mediasoup_build_test_support
            mediasoup_build_pthread
            hiredis
    )

    mediasoup_add_configured_executable(mediasoup_bench
        SOURCES tests/bench_worker_load.cpp
        LINK_LIBRARIES
            mediasoup_lib
            mediasoup_build_pthread
    )

    mediasoup_add_gtest_target(mediasoup_qos_accuracy_tests
        SOURCES tests/test_qos_accuracy.cpp
        LINK_LIBRARIES
            mediasoup_build_test_support
            mediasoup_lib
            mediasoup_build_pthread
    )

    mediasoup_add_gtest_target(mediasoup_qos_recording_accuracy_tests
        SOURCES tests/test_qos_recording_accuracy.cpp
        LINK_LIBRARIES
            mediasoup_build_test_support
            mediasoup_lib
            mediasoup_build_pthread
            mediasoup_build_ffmpeg_core
    )

    mediasoup_add_gtest_target(mediasoup_thread_integration_tests
        SOURCES tests/test_thread_integration.cpp
        LINK_LIBRARIES
            mediasoup_build_test_support
            mediasoup_build_pthread
            mediasoup_build_ffmpeg_extended
    )
endif()
