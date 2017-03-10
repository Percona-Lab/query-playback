/* BEGIN LICENSE
 * Copyright (c) 2017 Dropbox, Inc.
 * Copyright (C) 2011-2013 Percona Ireland Ltd.
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
#include "general_log/general_log.h"

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/foreach.hpp>

#include <percona_playback/plugin.h>

#include <vector>

namespace po= boost::program_options;

percona_playback::DBClientPlugin *g_dbclient_plugin= NULL;
percona_playback::InputPlugin *g_input_plugin= NULL;
percona_playback::DispatcherPlugin *g_dispatcher_plugin= NULL;
unsigned int g_db_thread_queue_depth;
std::string g_session_init_query;

using namespace percona_playback;

struct percona_playback_st
{
  const char *name;
  unsigned int loop;
};

percona_playback_st *percona_playback_create(const char *name)
{
  percona_playback_st *the_percona_playback=
    static_cast<percona_playback_st *>(malloc(sizeof(percona_playback_st)));
  assert(the_percona_playback);
  the_percona_playback->name= name;
  return the_percona_playback;
}

void percona_playback_destroy(percona_playback_st **the_percona_playback)
{
  if (the_percona_playback)
  {
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
	    << _("Version: ") << PACKAGE_VERSION;

  if (strlen(PERCONA_PLAYBACK_RELEASE_COMMENT) > 0)
  {
    std::cerr << "-" << PERCONA_PLAYBACK_RELEASE_COMMENT;
  }

  std::cerr << std::endl;
}

static void help(po::options_description &options_description)
{
    version();
    std::cerr << std::endl;
    std::cerr << options_description << std::endl;
    std::cerr << std::endl;
    std::cerr << _("Bugs: ") << PACKAGE_BUGREPORT << std::endl;
    std::cerr << _("Loaded plugins: ");
    BOOST_FOREACH(const std::string &plugin_name, PluginRegistry::singleton().loaded_plugin_names)
    {
      std::cerr << plugin_name << " ";
    }

    std::cerr << std::endl;

    std::cerr << std::endl << _("Loaded DB Plugins: ");
    for(PluginRegistry::DBClientPluginMap::iterator it= PluginRegistry::singleton().dbclient_plugins.begin();
	it != PluginRegistry::singleton().dbclient_plugins.end();
	++it)
    {
      std::cerr << it->first << " ";
    }
    std::cerr << std::endl;
    std::cerr << std::endl;

    assert(g_dbclient_plugin);
    std::cerr << _("Selected DB Plugin: ") << g_dbclient_plugin->name << std::endl;

    std::cerr << std::endl << _("Loaded Input Plugins: ");

    BOOST_FOREACH(const PluginRegistry::InputPluginPair &pp,
		  PluginRegistry::singleton().input_plugins)
    {
      std::cerr << pp.first << " ";
    }

    std::cerr << std::endl;
    std::cerr << std::endl;

    assert(g_input_plugin);
    std::cerr << _("Selected Input Plugin: ")
              << g_input_plugin->name
              << std::endl;

    std::cerr << std::endl << _("Loaded Dispatcher Plugins: ");

    BOOST_FOREACH(const PluginRegistry::DispatcherPluginPair &pp,
		  PluginRegistry::singleton().dispatcher_plugins)
    {
      std::cerr << pp.first << " ";
    }

    std::cerr << std::endl;
    std::cerr << std::endl;

    assert(g_dispatcher_plugin);
    std::cerr << _("Selected dispatcher Plugin: ")
              << g_dispatcher_plugin->name
              << std::endl;

    std::cerr << std::endl << _("Loaded Reporting Plugins: ");

    BOOST_FOREACH(const PluginRegistry::ReportPluginPair &pp,
                  PluginRegistry::singleton().report_plugins)
    {
      std::cerr << pp.first << " ";
    }

    std::cerr << std::endl;
    std::cerr << std::endl;

    std::cerr << _("Selected reporting Plugins: ");
    BOOST_FOREACH(const PluginRegistry::ReportPluginPair &pp,
                  PluginRegistry::singleton().report_plugins)
    {
      if (pp.second->active)
        std::cerr << pp.first << " ";
    }
    std::cerr << std::endl;
}

int percona_playback_argv(percona_playback_st *the_percona_playback,
			  int argc, char** argv)
{
  load_plugins();

  po::options_description general_options(_("General options"));
  general_options.add_options()
    ("help",    _("Display this message"))
    ("version", _("Display version information"))
    /* We will re-enable this "soon"
    ("loop", po::value<unsigned int>(), _("Do the whole run N times"))
    */
    ;

  po::options_description db_options("Database Options");
  db_options.add_options()
    ("db-plugin", po::value<std::string>(), _("Database plugin"))
    ("input-plugin", po::value<std::string>(), _("Input plugin"))
    ("dispatcher-plugin", po::value<std::string>(), _("Dispatcher plugin"))
    ("disable-reporting-plugin", po::value<std::vector<std::string> >(), _("Disable reporting plugin"))
    ("queue-depth", po::value<unsigned int>(),
     _("Queue depth for DB executor (thread). The larger this number the"
     " greater the played-back workload can deviate from the original workload"
     " as some connections may be up to queue-depth behind. (default 1)"))
    ("session-init-query",
     po::value<std::string>(&g_session_init_query)->default_value(""),
     _("This query will be executed just after each connect to db"))
    ;

  std::string basic_usage;
  basic_usage= _("USAGE: ") + std::string(PACKAGE) + _(" [General Options]");
  po::options_description options_description(basic_usage);
  options_description.add(general_options);
  options_description.add(db_options);

  BOOST_FOREACH(const PluginRegistry::PluginPair pp,
		PluginRegistry::singleton().all_plugins)
  {
    po::options_description *plugin_opts= pp.second->getProgramOptions();

    if (plugin_opts != NULL)
      options_description.add(*plugin_opts);
  }

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options_description), vm);
  po::notify(vm);

  if (vm.count("db-plugin"))
  {
    PluginRegistry::DBClientPluginMap::iterator it;
    it= PluginRegistry::singleton().dbclient_plugins.find(vm["db-plugin"].as<std::string>());
    if (it == PluginRegistry::singleton().dbclient_plugins.end())
    {
      fprintf(stderr, _("Invalid DB Plugin\n"));
      return -1;
    }
    g_dbclient_plugin= it->second;
  }
  else
  {
    PluginRegistry::DBClientPluginMap::iterator it;
    it= PluginRegistry::singleton().dbclient_plugins.find("libmysqlclient");
    if (it == PluginRegistry::singleton().dbclient_plugins.end())
    {
      PluginRegistry::DBClientPluginMap::iterator null_it;
      null_it= PluginRegistry::singleton().dbclient_plugins.find("null");
      if (null_it == PluginRegistry::singleton().dbclient_plugins.end())
      {
	fprintf(stderr, _("Invalid DB plugin\n"));
	return -1;
      }
      g_dbclient_plugin= null_it->second;
    }
    else
    {
      g_dbclient_plugin= it->second;
    }
  }
  g_dbclient_plugin->active= true;

  if (vm.count("input-plugin"))
  {
    PluginRegistry::InputPluginMap::iterator it;
    it= PluginRegistry::singleton().input_plugins.find(vm["input-plugin"].as<std::string>());
    if (it == PluginRegistry::singleton().input_plugins.end())
    {
      fprintf(stderr, _("Invalid Input Plugin\n"));
      return -1;
    }
    g_input_plugin= it->second;
  }
  else
  {
    PluginRegistry::InputPluginMap::iterator it;
    it= PluginRegistry::singleton().input_plugins.find("query-log");
    if (it == PluginRegistry::singleton().input_plugins.end())
    {
      fprintf(stderr, _("Invalid Input plugin\n"));
      return -1;
    }
    g_input_plugin= it->second;
  }
  g_input_plugin->active= true;

  if (vm.count("dispatcher-plugin"))
  {
    PluginRegistry::DispatcherPluginMap::iterator it;
    it= PluginRegistry::singleton().dispatcher_plugins.find(
      vm["dispatcher-plugin"].as<std::string>());
    if (it == PluginRegistry::singleton().dispatcher_plugins.end())
    {
      fprintf(stderr, _("Invalid Dispatcher Plugin\n"));
      return -1;
    }
    g_dispatcher_plugin= it->second;
  }
  else
  {
    PluginRegistry::DispatcherPluginMap::iterator it;
    it= PluginRegistry::singleton().dispatcher_plugins.
      find("thread-per-connection");
    if (it == PluginRegistry::singleton().dispatcher_plugins.end())
    {
      fprintf(stderr, _("Invalid Dispatcher plugin\n"));
      return -1;
    }
    g_dispatcher_plugin= it->second;
  }
  g_dispatcher_plugin->active= true;

  if (vm.count("disable-reporting-plugin"))
  {
    std::vector<std::string> plugins = vm["disable-reporting-plugin"].as<std::vector<std::string> >();
    for (std::vector<std::string>::iterator it = plugins.begin(), end = plugins.end(); it != end; ++it) {
      PluginRegistry::ReportPluginMap::iterator plugin_it;
      plugin_it = PluginRegistry::singleton().report_plugins.find(*it);
      if (plugin_it == PluginRegistry::singleton().report_plugins.end())
      {
        fprintf(stderr, _("Invalid Reporting plugin: '%s'\n"), it->c_str());
        return -1;
      }
      plugin_it->second->active = false;
    }
  }

  if (vm.count("help") || argc==1)
  {
    help(options_description);
    return 1;
  }

  if (vm.count("version"))
  {
    version();
    return 2;
  }

  /*
    Process plugin options after "help" processing to avoid
    required options requests in "help" message.
  */
  BOOST_FOREACH(const PluginRegistry::PluginPair &pp,
		PluginRegistry::singleton().all_plugins)
  {
    if (pp.second->processOptions(vm))
      return -1;
  }

  if (vm.count("loop"))
  {
    the_percona_playback->loop= vm["loop"].as<unsigned int>();
  }
  else
    the_percona_playback->loop= 1;

  if (vm.count("queue-depth"))
  {
    g_db_thread_queue_depth= vm["queue-depth"].as<unsigned int>();
  }
  else
    g_db_thread_queue_depth= 1;

  return 0;
}

static
percona_playback_run_result *
create_percona_playback_run_result()
{
  percona_playback_run_result *r=
    static_cast<struct percona_playback_run_result *>(
      malloc(sizeof(struct percona_playback_run_result)));
  assert(r);
  r->err= 0;
  r->n_log_entries= 0;
  r->n_queries= 0;
  return r;
}

struct percona_playback_run_result *percona_playback_run(const percona_playback_st *)
{
  percona_playback_run_result *r= create_percona_playback_run_result();
  assert(g_dbclient_plugin);

  std::cerr << _("Database Plugin: ") << g_dbclient_plugin->name << std::endl;
  std::cerr << _(" Running...") << std::endl;

  g_dispatcher_plugin->run();
  g_input_plugin->run(*r);

  BOOST_FOREACH(const PluginRegistry::ReportPluginPair pp,
		  PluginRegistry::singleton().report_plugins)
  {
    if (pp.second->active)
      pp.second->print_report();
  }

  return r;
}

int percona_playback_run_all(const percona_playback_st *the_percona_playback)
{
  struct percona_playback_run_result *r;

  for(unsigned int run=0; run < the_percona_playback->loop; run++)
  {
    if (the_percona_playback->loop > 1)
    {
      fprintf(stderr, _("Run %u of %u\n"), run+1, the_percona_playback->loop);
    }
    r= percona_playback_run(the_percona_playback);
    if (r->err != 0)
    {
      free(r);
      return -1;
    }
    free(r);
  }

  return 0;
}
