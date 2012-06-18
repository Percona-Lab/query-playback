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

#ifndef PERCONA_PLAYBACK_DB_THREAD_H
#define PERCONA_PLAYBACK_DB_THREAD_H

#include "percona_playback/visibility.h"
#include "boost/thread.hpp"
#include "tbb/concurrent_queue.h"
#include "tbb/concurrent_hash_map.h"

#include "percona_playback/query_entry.h"

#ifdef __cplusplus
extern "C"
{
#endif

class DBThread;
void RunDBThread(DBThread* dbt, uint64_t thread_id);

typedef tbb::concurrent_hash_map<uint64_t, DBThread*> DBExecutorsTable;

extern DBExecutorsTable db_executors;
extern unsigned int g_db_thread_queue_depth;

class QueryResult;

class DBThreadState
{
public:
  virtual ~DBThreadState(){}
};

class DBThread
{

private:
  boost::thread thread;
  uint64_t thread_id;
  DBThreadState *state;
  bool manage_state;

public:
  typedef tbb::concurrent_bounded_queue<QueryEntryPtr> Queries;
  Queries queries;

  DBThread(uint64_t _thread_id, bool manage_state= true) :
    thread_id(_thread_id),
    state(NULL),
    manage_state(manage_state)
  {
    queries.set_capacity(g_db_thread_queue_depth);
  }

  ~DBThread() {
    if (manage_state)
      delete state;
  }

  void join()
  {
    thread.join();
  }

  virtual void connect()= 0;

  virtual void disconnect()= 0;
  virtual void execute_query(const std::string &query,
			     QueryResult *r,
			     const QueryResult &expected_result)= 0;

  bool run();

  void start_thread()
  {
    thread= boost::thread(RunDBThread, this, thread_id);
  }
};

#ifdef __cplusplus
}
#endif

#endif /* PERCONA_PLAYBACK_DB_THREAD_H */
