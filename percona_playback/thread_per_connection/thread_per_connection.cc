/* BEGIN LICENSE
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

#include <tbb/concurrent_hash_map.h>

class ThreadPerConnectionDispatcher :
	public percona_playback::DispatcherPlugin
{
  typedef tbb::concurrent_hash_map<uint64_t, DBThread*> DBExecutorsTable;
  DBExecutorsTable  executors;
  void db_thread_func(DBThread *thread);
  void start_thread(DBThread *thread);

public:
  ThreadPerConnectionDispatcher(std::string _name) :
	  DispatcherPlugin(_name) {}

  void dispatch(QueryEntryPtr query_entry);
  bool finish_and_wait(uint64_t thread_id);
  void finish_all_and_wait();
};

extern percona_playback::DBClientPlugin *g_dbclient_plugin;

void
ThreadPerConnectionDispatcher::dispatch(QueryEntryPtr query_entry)
{
  uint64_t thread_id= query_entry->getThreadId();
  {
    DBExecutorsTable::accessor a;
    if (executors.insert(a, thread_id))
    {
      DBThread *db_thread= g_dbclient_plugin->create(thread_id);
      a->second= db_thread;
      db_thread->start_thread();
    }
    a->second->queries->push(query_entry);
  }
}

bool
ThreadPerConnectionDispatcher::finish_and_wait(uint64_t thread_id)
{
  DBThread *db_thread= NULL;
  {
    DBExecutorsTable::accessor a;
    if (executors.find(a, thread_id))
    {
      db_thread= a->second;
      executors.erase(a);
    }
  }

  if (!db_thread)
    return false;

  db_thread->queries->push(QueryEntryPtr(new FinishEntry(thread_id)));
  db_thread->join();

  delete db_thread;

  return true;
}

void
ThreadPerConnectionDispatcher::finish_all_and_wait()
{
  QueryEntryPtr shutdown_command(new FinishEntry(0));

  while(executors.size())
  {
    uint64_t thread_id;
    DBThread *t;
    {
      DBExecutorsTable::const_iterator iter= executors.begin();
      thread_id= (*iter).first;
      t= (*iter).second;
    }
    executors.erase(thread_id);

    t->queries->push(shutdown_command);
    t->join();

    delete t;
  }
}

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("thread-per-connection",
	new ThreadPerConnectionDispatcher("thread-per-connection"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
