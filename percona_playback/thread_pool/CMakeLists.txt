list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/percona_playback")
include(plugin)

add_library(thread_pool thread_pool.cc)

setup_plugin("thread_pool")
