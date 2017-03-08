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

cmake_minimum_required(VERSION 2.8.7)

enable_testing()

set(MAJOR_VERSION 0)
set(MINOR_VERSION 7)
set(PATCH_VERSION 0)

project("percona-playback")

set(VERSION "${MAJOR_VERSION}.${MINOR_VERSION}")
set(PACKAGE "${PROJECT_NAME}")
set(BUGREPORT "https://github.com/Percona-Lab/query-playback")

include_directories(${PROJECT_BINARY_DIR})

# code for generating config.h
configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_BINARY_DIR}/config.h")

add_subdirectory(doxygen)
add_subdirectory(percona_playback)


# main percona-playback executable
include_directories(${PROJECT_SOURCE_DIR})
add_executable(percona-playback bin/percona_playback.cc)
target_link_libraries(percona-playback libpercona-playback ssl crypto)
install(TARGETS percona-playback RUNTIME DESTINATION bin LIBRARY DESTINATION lib)

# let one use 'make check' as a synonym for 'make test'
add_custom_target(check COMMAND ${CMAKE_BUILD_TOOL} test)

# packaging metadata
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libtbb2, libboost-dev, libmysqlclient18 | libmysqlclient20")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/Percona-Lab/query-playback")
set(CPACK_DEBIAN_PACKAGE_SECTION "Applications/Databases")
set(CPACK_PACKAGE_CONTACT "https://github.com/Percona-Lab/query-playback")
set(CPACK_PACKAGE_DESCRIPTION "Percona Playback is a tool for replaying the captured load of one database "
                              "server against another in the most realistic way possible. Captured load can "
                              "come in the form of MySQL query logs.  It's multithreaded, modular and "
                              "configurable to allow for flexibility and future extension.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A tool for replaying captured database server load")
set(CPACK_PACKAGE_VENDOR "Percona Lab")
set(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")
include(CPack)
