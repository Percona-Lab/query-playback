list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/percona_playback")
include(plugin)

add_library(general_log parse_general_log.cc general_log_entry.cc general_log.cc)

setup_plugin("general_log")
