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

#include "percona_playback/plugin.h"
#include "percona_playback/db_thread.h"

#include <boost/unordered_map.hpp>

extern percona_playback::DBClientPlugin *g_dbclient_plugin;

class ThreadPerConnectionDispatcher :
        public percona_playback::DispatcherPlugin
{
  typedef boost::unordered_map<uint64_t, DBThread*> DBExecutorsTable;
  DBExecutorsTable  executors;

public:
  ThreadPerConnectionDispatcher(std::string _name) :
          DispatcherPlugin(_name) {}

  void dispatch(QueryEntriesPtr query_entries);
  void finish_all_and_wait();
};

void
ThreadPerConnectionDispatcher::dispatch(QueryEntriesPtr query_entries)
{
  // automatically close threads after last request
  query_entries->setShutdownOnLastQueryOfConn();

  while (QueryEntryPtr query_entry = query_entries->popEntry()) {
    uint64_t thread_id = query_entry->getThreadId();
    DBThread*& db_thread = executors[thread_id];
    if (!db_thread) {
      db_thread = g_dbclient_plugin->create(thread_id);
      db_thread->start_thread();
    }
    db_thread->queries->push(query_entry);
  }
}

void
ThreadPerConnectionDispatcher::finish_all_and_wait()
{
  for (DBExecutorsTable::iterator it = executors.begin(), end = executors.end(); it != end; ++it) {
    it->second->join();
    delete it->second;
  }
}

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("thread-per-connection",
        new ThreadPerConnectionDispatcher("thread-per-connection"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
