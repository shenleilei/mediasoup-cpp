include_guard(GLOBAL)

function(resolve_ffmpeg_include_dirs out_var)
  set(_ffmpeg_include_dirs "")

  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(FFMPEG QUIET libavformat libavcodec libavutil libswscale libavdevice)
    if(FFMPEG_INCLUDE_DIRS)
      list(APPEND _ffmpeg_include_dirs ${FFMPEG_INCLUDE_DIRS})
    endif()
  endif()

  if(DEFINED FFMPEG_INCLUDE_DIRS AND NOT "${FFMPEG_INCLUDE_DIRS}" STREQUAL "")
    list(APPEND _ffmpeg_include_dirs ${FFMPEG_INCLUDE_DIRS})
  endif()

  if(DEFINED ENV{FFMPEG_INCLUDE_DIRS} AND NOT "$ENV{FFMPEG_INCLUDE_DIRS}" STREQUAL "")
    list(APPEND _ffmpeg_include_dirs $ENV{FFMPEG_INCLUDE_DIRS})
  endif()

  if(DEFINED FFMPEG_ROOT AND NOT "${FFMPEG_ROOT}" STREQUAL "")
    list(APPEND _ffmpeg_include_dirs
      "${FFMPEG_ROOT}/include"
      "${FFMPEG_ROOT}/include/ffmpeg")
  endif()

  if(DEFINED ENV{FFMPEG_ROOT} AND NOT "$ENV{FFMPEG_ROOT}" STREQUAL "")
    list(APPEND _ffmpeg_include_dirs
      "$ENV{FFMPEG_ROOT}/include"
      "$ENV{FFMPEG_ROOT}/include/ffmpeg")
  endif()

  list(APPEND _ffmpeg_include_dirs
    /usr/include
    /usr/include/ffmpeg
    /usr/local/include
    /usr/local/include/ffmpeg)

  list(REMOVE_DUPLICATES _ffmpeg_include_dirs)

  find_path(FFMPEG_HEADER_DIR
    NAMES libavformat/avformat.h
    HINTS ${_ffmpeg_include_dirs})

  if(NOT FFMPEG_HEADER_DIR)
    message(FATAL_ERROR
      "FFmpeg headers not found. Install FFmpeg development packages or set "
      "FFMPEG_INCLUDE_DIRS / FFMPEG_ROOT.")
  endif()

  set(${out_var} "${FFMPEG_HEADER_DIR}" PARENT_SCOPE)
endfunction()
