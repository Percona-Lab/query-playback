add_subdirectory(test)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# setup db_thread lib which all plugins depend on
find_package(Boost COMPONENTS system thread regex program_options REQUIRED)
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
  get_filename_component(plugin_path ${plugin_cmakefile} DIRECTORY)
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
