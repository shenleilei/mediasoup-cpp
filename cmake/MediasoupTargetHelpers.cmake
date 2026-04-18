include_guard(GLOBAL)

function(mediasoup_add_interface_target target)
  set(multiValueArgs INCLUDE_DIRS LINK_LIBRARIES)
  cmake_parse_arguments(MEDIASOUP "" "" "${multiValueArgs}" ${ARGN})

  if(TARGET ${target})
    message(FATAL_ERROR "Target '${target}' already exists")
  endif()

  add_library(${target} INTERFACE)

  if(MEDIASOUP_INCLUDE_DIRS)
    target_include_directories(${target} INTERFACE ${MEDIASOUP_INCLUDE_DIRS})
  endif()

  if(MEDIASOUP_LINK_LIBRARIES)
    target_link_libraries(${target} INTERFACE ${MEDIASOUP_LINK_LIBRARIES})
  endif()
endfunction()

function(mediasoup_add_configured_executable target)
  set(multiValueArgs SOURCES INCLUDE_DIRS LINK_LIBRARIES)
  cmake_parse_arguments(MEDIASOUP "" "" "${multiValueArgs}" ${ARGN})

  if(NOT MEDIASOUP_SOURCES)
    message(FATAL_ERROR "mediasoup_add_configured_executable(${target}) requires SOURCES")
  endif()

  add_executable(${target} ${MEDIASOUP_SOURCES})

  if(MEDIASOUP_INCLUDE_DIRS)
    target_include_directories(${target} PRIVATE ${MEDIASOUP_INCLUDE_DIRS})
  endif()

  if(MEDIASOUP_LINK_LIBRARIES)
    target_link_libraries(${target} PRIVATE ${MEDIASOUP_LINK_LIBRARIES})
  endif()
endfunction()

function(mediasoup_add_gtest_target target)
  set(oneValueArgs WORKING_DIRECTORY)
  set(multiValueArgs SOURCES INCLUDE_DIRS LINK_LIBRARIES)
  cmake_parse_arguments(MEDIASOUP "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT MEDIASOUP_SOURCES)
    message(FATAL_ERROR "mediasoup_add_gtest_target(${target}) requires SOURCES")
  endif()

  mediasoup_add_configured_executable(${target}
    SOURCES ${MEDIASOUP_SOURCES}
    INCLUDE_DIRS ${MEDIASOUP_INCLUDE_DIRS}
    LINK_LIBRARIES gtest gtest_main ${MEDIASOUP_LINK_LIBRARIES}
  )

  if(NOT MEDIASOUP_WORKING_DIRECTORY)
    set(MEDIASOUP_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  endif()

  add_test(NAME ${target} COMMAND ${target}
    WORKING_DIRECTORY ${MEDIASOUP_WORKING_DIRECTORY})
endfunction()
