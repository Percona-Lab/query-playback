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
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_hash_map.h>

#include "percona_playback/query_entry.h"

#ifdef __cplusplus
extern "C"
{
#endif

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
  boost::shared_ptr<DBThreadState> state;

public:
  typedef tbb::concurrent_bounded_queue<QueryEntryPtr> Queries;
  Queries queries;

  DBThread(uint64_t _thread_id) : thread_id(_thread_id)  {
    queries.set_capacity(g_db_thread_queue_depth);
  }

  virtual ~DBThread() {}

  void set_state(boost::shared_ptr<DBThreadState> new_state)
  {
    state= new_state;
  }

  boost::shared_ptr<DBThreadState> get_state() const
  {
    return state;
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

  void run();

  void start_thread();
};

#ifdef __cplusplus
}
#endif

#endif /* PERCONA_PLAYBACK_DB_THREAD_H */
