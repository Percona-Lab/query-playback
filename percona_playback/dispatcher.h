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

#ifndef PERCONA_PLAYBACK_DISPATCHER_H
#define PERCONA_PLAYBACK_DISPATCHER_H

#include <tbb/concurrent_hash_map.h>
#include <boost/function.hpp>

#include "percona_playback/query_entry.h"

class DBThread;
class DBThreadState;

class Dispatcher
{
  typedef tbb::concurrent_hash_map<uint64_t, DBThread*> DBExecutorsTable;
  DBExecutorsTable  executors;
  void db_thread_func(DBThread *thread);
  void start_thread(DBThread *thread);

public:
  boost::shared_ptr<DBThreadState>
    get_thread_state(uint64_t thread_id,
                     boost::function1<void, DBThread *>
                      run_on_db_thread_create=
                        boost::function1<void, DBThread *>());
  void dispatch(QueryEntryPtr query_entry);
  bool finish_and_wait(uint64_t thread_id);
  void finish_all_and_wait();
};

extern Dispatcher g_dispatcher;

#endif /* PERCONA_PLAYBACK_DISPATCHER_H */
