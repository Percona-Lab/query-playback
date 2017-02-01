list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/percona_playback")
include(plugin)

add_library(simple_report simple_report.cc)

setup_plugin("simple_report")
