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
