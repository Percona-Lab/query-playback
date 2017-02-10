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

function(read_ini_key ini_text key outputvar)
  string(REGEX REPLACE "(.*)${key}=([^\n]+)(.*)" "\\2" OUTPUTVAR "${ini_text}")
endfunction(read_ini_key)

function(setup_plugin plugin_name)
  file(STRINGS plugin.ini ini_text NEWLINE_CONSUME)

  read_ini_key(${ini_text} "title" plugin_title)
  read_ini_key(${ini_text} "version" plugin_version)
  read_ini_key(${ini_text} "author" plugin_author)
 
  include_directories("${CMAKE_SOURCE_DIR}")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/percona_playback")

  add_definitions(-DPANDORA_MODULE_VERSION="${plugin_version}")
  add_definitions(-DPANDORA_MODULE_AUTHOR="${plugin_author}")
  add_definitions(-DPANDORA_MODULE_TITLE="${plugin_title}")
  add_definitions(-DPANDORA_MODULE_NAME=${plugin_name})
  add_definitions(-DPANDORA_MODULE_LICENCE="PLUGIN_LICENSE_GPL")

  target_link_libraries(${plugin_name} db_thread)

  # only install the plugin if we compile it as a shared library
  if(${BUILD_SHARED_LIBS})
    install(TARGETS ${plugin_name} DESTINATION lib)
  endif(${BUILD_SHARED_LIBS})
endfunction(setup_plugin)
