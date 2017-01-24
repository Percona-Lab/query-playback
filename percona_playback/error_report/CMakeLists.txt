list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/percona_playback")
include(plugin)

add_library(error_report error_report.cc)

setup_plugin("error_report")
