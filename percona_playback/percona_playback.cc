/* BEGIN LICENSE
 * Copyright (C) 2011 Percona Inc.
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 * END LICENSE */

#include "config.h"

#include <stdlib.h>
#include <string>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <assert.h>
#include "percona_playback.h"
#include "version.h"
#include "query_log/query_log.h"

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/foreach.hpp>

#include <percona_playback/plugin.h>

#include <vector>

namespace po= boost::program_options;

percona_playback::DBClientPlugin *g_dbclient_plugin;

struct percona_playback_st
{
  const char *name;
  unsigned int loop;
  std::vector<std::string> *slow_query_log_files;
  unsigned int query_log_file_read_count;
};

percona_playback_st *percona_playback_create(const char *name)
{
  percona_playback_st *the_percona_playback=
    static_cast<percona_playback_st *>(malloc(sizeof(percona_playback_st)));
  assert(the_percona_playback);
  the_percona_playback->name= name;
  the_percona_playback->slow_query_log_files= NULL;
  the_percona_playback->query_log_file_read_count= 0;
  return the_percona_playback;
}

void percona_playback_destroy(percona_playback_st **the_percona_playback)
{
  if (the_percona_playback)
  {
    delete (*the_percona_playback)->slow_query_log_files;
    free(*the_percona_playback);
    *the_percona_playback= NULL;
  }
}

const char *percona_playback_get_name(const percona_playback_st *the_percona_playback)
{
  if (the_percona_playback)
    return the_percona_playback->name;
  return NULL;
}

static void version()
{
  std::cerr << PACKAGE << std::endl
	    << "Version: " PACKAGE_VERSION
	    << "-" << PERCONA_PLAYBACK_VERSION_ID
	    << "-" << PERCONA_PLAYBACK_RELEASE_COMMENT
	    << std::endl;
}

int percona_playback_argv(percona_playback_st *the_percona_playback,
			  int argc, char** argv)
{
  percona_playback::load_plugins();

  po::options_description general_options("General options");
  general_options.add_options()
    ("help",    "Display this message")
    ("version", "Display version information")
    ("loop", po::value<unsigned int>(), "Do the whole run N times")
    ;

  po::options_description query_log_options("Query Log Options");
  query_log_options.add_options()
    ("slow-query-log-file", po::value<std::string>(), "Query log file")
    ("file-read-count", po::value<unsigned int>(), "Query log file read count (how many times to read query log file)")
    ;

  po::options_description db_options("Database Options");
  db_options.add_options()
    ("db-plugin", po::value<std::string>(), "Database plugin")
    ;

  std::string basic_usage;
  basic_usage= "USAGE: " + std::string(PACKAGE) + " [General Options]";
  po::options_description options_description(basic_usage);
  options_description.add(general_options);
  options_description.add(query_log_options);
  options_description.add(db_options);

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options_description), vm);
  po::notify(vm);

  if (vm.count("db-plugin"))
  {
    g_dbclient_plugin= percona_playback::PluginRegistry::singleton().dbclient_plugins[vm["db-plugin"].as<std::string>()];
    if (g_dbclient_plugin == NULL)
    {
      std::cerr << "Invalid DB Plugin" << std::endl;
      return -1;
    }
  }
  else
    g_dbclient_plugin= percona_playback::PluginRegistry::singleton().dbclient_plugins["libmysqlclient_mysql_client"];

  if (vm.count("help") || argc==1)
  {
    version();
    std::cerr << std::endl;
    std::cerr << options_description << std::endl;
    std::cerr << std::endl;
    std::cerr << "Bugs: " << PACKAGE_BUGREPORT << std::endl;
    std::cerr << "Loaded plugins: ";
    BOOST_FOREACH(const std::string &plugin_name, percona_playback::PluginRegistry::singleton().loaded_plugin_names)
    {
      std::cerr << plugin_name << " ";
    }

    std::cerr << std::endl;

    std::cerr << std::endl << "Loaded DB Plugins: ";
    for(percona_playback::PluginRegistry::DBClientPluginMap::iterator it= percona_playback::PluginRegistry::singleton().dbclient_plugins.begin();
	it != percona_playback::PluginRegistry::singleton().dbclient_plugins.end();
	it++)
    {
      std::cerr << it->first << " ";
    }
    std::cerr << std::endl;
    std::cerr << std::endl;

    std::cerr << "Selected DB Plugin: " << g_dbclient_plugin->name << std::endl;

    return 1;
  }

  if (vm.count("version"))
  {
    version();
    return 2;
  }
  if (vm.count("loop"))
  {
    the_percona_playback->loop= vm["loop"].as<unsigned int>();
  }
  else
    the_percona_playback->loop= 1;

  if (vm.count("slow-query-log-file"))
  {
    the_percona_playback->slow_query_log_files= new std::vector<std::string>();
    the_percona_playback->slow_query_log_files->push_back(vm["slow-query-log-file"].as<std::string>());
  }
  else
    the_percona_playback->slow_query_log_files= NULL;

  if (vm.count("file-read-count"))
  {
    the_percona_playback->query_log_file_read_count= vm["file-read-count"].as<unsigned int>();
  }
  else
    the_percona_playback->query_log_file_read_count= 1;

  return 0;
}

int percona_playback_run(const percona_playback_st *the_percona_playback)
{
  std::cerr << "Database Plugin: " << g_dbclient_plugin->name << std::endl;
  std::cerr << " Running..." << std::endl;
  std::cerr << "  Query Log File: "
	    << (*the_percona_playback->slow_query_log_files)[0]
	    << std::endl;

  return run_query_log((*the_percona_playback->slow_query_log_files)[0],
		       the_percona_playback->query_log_file_read_count);
}

int percona_playback_run_all(const percona_playback_st *the_percona_playback)
{
  int r=0;

  for(unsigned int run=0; run < the_percona_playback->loop; run++)
  {
    if (the_percona_playback->loop > 1)
    {
      fprintf(stderr, "Run %u of %u\n", run+1, the_percona_playback->loop);
    }
    r= percona_playback_run(the_percona_playback);
    if (r != 0)
      return -1;
  }

  return 0;
}
