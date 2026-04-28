include_guard(GLOBAL)

set(MEDIASOUP_COMMON_FFMPEG_RELATIVE_SOURCES
  common/ffmpeg/AvError.cpp
  common/ffmpeg/InputFormat.cpp
  common/ffmpeg/OutputFormat.cpp
  common/ffmpeg/Decoder.cpp
  common/ffmpeg/Encoder.cpp
  common/ffmpeg/BitstreamFilter.cpp
)

function(mediasoup_collect_common_ffmpeg_sources out_var repo_root)
  if(NOT IS_ABSOLUTE "${repo_root}")
    get_filename_component(_repo_root "${repo_root}" ABSOLUTE)
  else()
    set(_repo_root "${repo_root}")
  endif()

  set(_sources)
  foreach(_relative_source IN LISTS MEDIASOUP_COMMON_FFMPEG_RELATIVE_SOURCES)
    list(APPEND _sources "${_repo_root}/${_relative_source}")
  endforeach()

  set(${out_var} "${_sources}" PARENT_SCOPE)
endfunction()
