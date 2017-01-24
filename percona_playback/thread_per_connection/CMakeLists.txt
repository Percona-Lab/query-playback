list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/percona_playback")
include(plugin)

add_library(thread_per_connection thread_per_connection.cc)
target_link_libraries(thread_per_connection tbb)

setup_plugin("thread_per_connection")
