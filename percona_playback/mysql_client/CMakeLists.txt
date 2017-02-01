list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/percona_playback")
include(plugin)

find_path(MYSQL_INC "mysql/mysql.h")
include_directories("${MYSQL_INC}/mysql")

add_library(mysql_client mysql_client.cc)
target_link_libraries(mysql_client mysqlclient_r)

setup_plugin("mysql_client")
