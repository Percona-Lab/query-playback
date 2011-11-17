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

#include "percona_playback/query_log/query_log.h"

#ifdef __cplusplus
extern "C"
{
#endif

class DBThread;
void RunDBThread(DBThread* dbt, uint64_t thread_id);

typedef tbb::concurrent_hash_map<uint64_t, DBThread*> DBExecutorsTable;

extern DBExecutorsTable db_executors;

class DBThread {
private:
  boost::thread *thread;
  uint64_t thread_id;
public:
  tbb::concurrent_bounded_queue<QueryLogEntry> queries;

  DBThread(uint64_t _thread_id) : thread_id(_thread_id) {
    queries.set_capacity(1);
  }

  ~DBThread() {
    delete thread;
  }

  void join()
  {
    thread->join();
  }

  virtual void connect()= 0;

  virtual void disconnect()= 0;
  virtual void execute_query(const std::string &query)= 0;

  bool run();

  void start_thread()
  {
    thread= new boost::thread(RunDBThread, this, thread_id);
  }
};

#ifdef __cplusplus
}
#endif

#endif /* PERCONA_PLAYBACK_DB_THREAD_H */
