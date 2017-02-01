list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/percona_playback")
include(plugin)

add_library(null_dbclient null_dbclient.cc)

setup_plugin("null_dbclient")
