include_directories(${PROJECT_SOURCE_DIR})

list(APPEND tests "basic-multiline")
list(APPEND tests "basic-queue-depth")
list(APPEND tests "basic-stdin")
list(APPEND tests "basic")
list(APPEND tests "crashme-slow")
list(APPEND tests "help")
list(APPEND tests "preserve_query_time")
list(APPEND tests "sqlbench-transactions-slow")
list(APPEND tests "sysbench-slow")
list(APPEND tests "thread-pool-sysbench-slow")

FOREACH(test ${tests})
  add_executable(test-${test} "${test}.cc")
  target_link_libraries(test-${test} libpercona-playback)
  add_definitions(-DSRCDIR="${PROJECT_SOURCE_DIR}")
 
  # we need to undefine NDEBUG so that assert don't get removed
  add_definitions(-UNDEBUG)

  add_test(test-${test} ${PROJECT_BINARY_DIR}/percona_playback/test/test-${test})
ENDFOREACH(test)
