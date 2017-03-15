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

add_subdirectory(test)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# setup db_thread lib which all plugins depend on
find_package(Boost 1.53.0 COMPONENTS system thread regex program_options chrono REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})
add_library(db_thread STATIC db_thread.cc)
target_link_libraries(db_thread ${Boost_LIBRARIES})

# write version.h
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/version.h")

# setup the percona-playback lib
add_library(libpercona-playback STATIC plugin.cc percona_playback.cc)
target_link_libraries(libpercona-playback "dl")

# static libraries linked into shared libraries need to be compiled with -fPIC
if(${BUILD_SHARED_LIBS})
  set_property(TARGET db_thread PROPERTY POSITION_INDEPENDENT_CODE ON)
  set_property(TARGET libpercona-playback PROPERTY POSITION_INDEPENDENT_CODE ON)
endif(${BUILD_SHARED_LIBS})

# build plugins
file(GLOB_RECURSE plugins "${CMAKE_CURRENT_LIST_DIR}//*/plugin.ini")
foreach(plugin_cmakefile ${plugins})
  get_filename_component(plugin_path ${plugin_cmakefile} PATH)
  get_filename_component(plugin_name ${plugin_path} NAME)
  
  message("Processing plugin ${plugin_name}")
  add_subdirectory(${plugin_path})
  link_directories(${plugin_path})
  target_link_libraries(libpercona-playback ${plugin_name})
  list(APPEND plugin_list ${plugin_name})
  list(APPEND plugin_sym_list "_percona_playback_${plugin_name}_plugin_")
endforeach(plugin_cmakefile)

# write plugin_load_list.h
string(REPLACE ";" "," PANDORA_BUILTIN_LIST "${plugin_list}")
set(PANDORA_BUILTIN_LOAD_LIST ${PANDORA_BUILTIN_LIST})
string(REPLACE ";" "," PANDORA_BUILTIN_SYMBOLS_LIST "${plugin_sym_list}")
set(PANDORA_BUILTIN_LOAD_SYMBOLS_LIST ${PANDORA_BUILTIN_SYMBOLS_LIST})
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/plugin_load_list.h.in" "${CMAKE_CURRENT_BINARY_DIR}/plugin_load_list.h")
