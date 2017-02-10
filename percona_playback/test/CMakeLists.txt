# Copyright (c) 2017 Dropbox, Inc.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  target_link_libraries(test-${test} libpercona-playback ssl crypto)
  add_definitions(-DSRCDIR="${PROJECT_SOURCE_DIR}")
 
  # we need to undefine NDEBUG so that assert don't get removed
  add_definitions(-UNDEBUG)

  add_test(test-${test} ${PROJECT_BINARY_DIR}/percona_playback/test/test-${test})
ENDFOREACH(test)
