include_guard(GLOBAL)

set(MEDIASOUP_COMMON_MEDIA_RELATIVE_SOURCES
  common/media/h264/AnnexB.cpp
  common/media/h264/Avcc.cpp
  common/media/rtp/H264Packetizer.cpp
)

function(mediasoup_collect_common_media_sources out_var repo_root)
  if(NOT IS_ABSOLUTE "${repo_root}")
    get_filename_component(_repo_root "${repo_root}" ABSOLUTE)
  else()
    set(_repo_root "${repo_root}")
  endif()

  set(_sources)
  foreach(_relative_source IN LISTS MEDIASOUP_COMMON_MEDIA_RELATIVE_SOURCES)
    list(APPEND _sources "${_repo_root}/${_relative_source}")
  endforeach()

  set(${out_var} "${_sources}" PARENT_SCOPE)
endfunction()
