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
  int num_running;

public:
  ThreadPerConnectionDispatcher(std::string _name) :
          DispatcherPlugin(_name) {}

  void dispatch(QueryEntriesPtr query_entries);
  void finish_all_and_wait();
  void finish_some_and_go();
};

void
ThreadPerConnectionDispatcher::dispatch(QueryEntriesPtr query_entries)
{
  // Keep track of how many threads we have started
  num_running = 0;

  // automatically close threads after last request
  query_entries->setShutdownOnLastQueryOfConn();

  while (QueryEntryPtr query_entry = query_entries->popEntry()) {
    uint64_t thread_id = query_entry->getThreadId();
    DBThread*& db_thread = executors[thread_id];
    if (!db_thread) {

      /* Under high load, it's possible to have too many threads
         running for the system. 10000 threads seems to be a good value
         to where we do not exhaust system resources, and maintain high
         throughput. Keeping the queue-depth at a high number is
         important to not get stuck waiting on long queries to finish.

         NOTE: It is technically possible to get stuck here forever, if
         there are actually 10000 threads that can't finish because at
         the time there are actually 10000 open transactions/connections
         that will now never get their shutdown on last query...

         But on workloads in the 2000-5000+ QPS range, this works
         much better. */

      // TODO: Make this 10000 an option &| do something if we seem stuck
      while ( num_running > 10000 ) {
        /* Allow some time for some threads to maybe finish */
        usleep(100000);
        /* Try to clean up threads that are done and
           bring the thread count down to within sane limits */
        ThreadPerConnectionDispatcher::finish_some_and_go();
      }

      db_thread = g_dbclient_plugin->create(thread_id);
      num_running += 1;
      db_thread->start_thread();
    }
    db_thread->queries->push(query_entry);

  }
}

void
ThreadPerConnectionDispatcher::finish_some_and_go()
{
  /* this is very similar to finish_all_and_wait(), except nonblocking_join()
     is now a try-for-join that only tries for 1ms. The idea is that we join up
     whatever threads are done and clean them out of the DBExecutorsTable
     quickly so we can get back to making new threads */
  for (DBExecutorsTable::iterator it = executors.begin(), end = executors.end(); it != end; ) {
    if (it->second && it->second->nonblocking_join()) {
      // FYI, erase(it) returns iterator to next element, so don't need to increment.
      it = executors.erase(it);
      num_running -= 1;
    } else {
      it++;
    }
  }
}

void
ThreadPerConnectionDispatcher::finish_all_and_wait()
{
  /* This is the final finish_all, and we DO want joins to actually join
     regardless of how long it takes to finish. Therefore, we use
     join() to block forever until the thread is done. */
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
