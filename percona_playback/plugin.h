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

#ifndef PERCONA_PLAYBACK_PLUGIN_H
#define PERCONA_PLAYBACK_PLUGIN_H

#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "percona_playback/query_entry.h"
#include <percona_playback/visibility.h>
#include <percona_playback/version.h>

// we currently don't translate strings
#define _(str) str

namespace boost {
  namespace program_options {
    class options_description;
    class variables_map;
  }
}

class DBThread;
class DBThreadState;
class QueryResult;
class percona_playback_run_result;

namespace percona_playback {

class PluginRegistry;

class plugin
{
 public:
  typedef struct
  {
    uint32_t api_version;
    const char* name;
    const char* version;
    const char* author;
    const char* title;
    const char* license;
    void (*init)(PluginRegistry &r);
  } definition;

  bool    active;

  plugin() : active(false) {}
  virtual ~plugin() {}

  virtual boost::program_options::options_description* getProgramOptions() {
    return NULL;
  }

  virtual int processOptions(boost::program_options::variables_map &) {
    return 0;
  }
};

class DBClientPlugin : public plugin
{
 public:
  std::string name;

  DBClientPlugin(std::string _name) : name(_name) {};

  virtual DBThread *create(uint64_t _thread_id)=0;
};

class ReportPlugin : public plugin
{
 public:
  std::string name;

  ReportPlugin(std::string _name) : name(_name) {}

  virtual void query_execution(const uint64_t thread_id,
			       const std::string &query,
			       const QueryResult &expected,
			       const QueryResult &actual)=0;

  virtual void print_report()=0;
};

class InputPlugin : public plugin
{
 public:
  std::string name;

  InputPlugin(const std::string &_name) : name(_name) {}

  virtual void run(percona_playback_run_result  &result)= 0;

};

class DispatcherPlugin : public plugin
{
 public:
  std::string name;

  DispatcherPlugin(const std::string &_name) : name(_name) {}

  virtual void dispatch(QueryEntriesPtr query_entries)= 0;
  virtual void finish_all_and_wait()= 0;

  virtual void run() {};

};

class PluginRegistry
{
 public:
  static PluginRegistry& singleton()
  {
    static PluginRegistry *registry= new PluginRegistry();
    return *registry;
  }

  std::vector<std::string> loaded_plugin_names;

  typedef std::map<std::string, DBClientPlugin*> DBClientPluginMap;
  typedef std::pair<std::string, DBClientPlugin*> DBClientPluginPair;

  typedef std::map<std::string, ReportPlugin*> ReportPluginMap;
  typedef std::pair<std::string, ReportPlugin*> ReportPluginPair;

  typedef std::map<std::string, InputPlugin*> InputPluginMap;
  typedef std::pair<std::string, InputPlugin*> InputPluginPair;

  typedef std::map<std::string, DispatcherPlugin*> DispatcherPluginMap;
  typedef std::pair<std::string, DispatcherPlugin*> DispatcherPluginPair;

  typedef std::map<std::string, plugin*> PluginMap;
  typedef std::pair<std::string, plugin*> PluginPair;

  PluginMap all_plugins;
  DBClientPluginMap dbclient_plugins;
  ReportPluginMap report_plugins;
  InputPluginMap input_plugins;
  DispatcherPluginMap dispatcher_plugins;

  void add(const std::string &name, plugin* plugin_object)
  {
    all_plugins.insert(PluginPair(name, plugin_object));
  }

  void add(const std::string &name, DBClientPlugin* dbclient)
  {
    dbclient_plugins.insert(DBClientPluginPair(name, dbclient));
    all_plugins.insert(PluginPair(name, dbclient));
  }

  void add(const std::string &name, ReportPlugin* report_plugin)
  {
    report_plugin->active= true;
    report_plugins.insert(ReportPluginPair(name, report_plugin));
    all_plugins.insert(PluginPair(name, report_plugin));
  } 

  void add(const std::string &name, InputPlugin* input_plugin)
  {
    input_plugins.insert(InputPluginPair(name, input_plugin));
    all_plugins.insert(PluginPair(name, input_plugin));
  }

  void add(const std::string &name, DispatcherPlugin* dispatcher_plugin)
  {
    dispatcher_plugins.insert(DispatcherPluginPair(name, dispatcher_plugin));
    all_plugins.insert(PluginPair(name, dispatcher_plugin));
  }

};


extern std::vector<std::string> loaded_plugin_names;
void load_plugins();

#define PERCONA_PLAYBACK_QUOTE(s) #s
#define PERCONA_PLAYBACK_QUOTE_VALUE(s) PERCONA_PLAYBACK_QUOTE(s)

#define PANDORA_CPP_NAME(x) _percona_playback_ ## x ## _plugin_
#define PANDORA_PLUGIN_NAME(x) PANDORA_CPP_NAME(x)

#define PERCONA_PLAYBACK_PLUGIN(init)			    \
  PERCONA_PLAYBACK_API percona_playback::plugin::definition	\
    PANDORA_PLUGIN_NAME(PANDORA_MODULE_NAME) =			\
    {							    \
      PERCONA_PLAYBACK_VERSION_ID,			    \
      PERCONA_PLAYBACK_QUOTE_VALUE(PANDORA_MODULE_NAME),    \
      PANDORA_MODULE_VERSION,				    \
      PANDORA_MODULE_AUTHOR,				    \
      PANDORA_MODULE_TITLE,				    \
      PERCONA_PLAYBACK_QUOTE_VALUE(PANDORA_MODULE_LICENSE), \
      init						    \
    }

} /* namespace percona_playback */

#endif /* PERCONA_PLAYBACK_PLUGIN_H */
