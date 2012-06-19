#include "percona_playback/dispatcher.h"
#include "percona_playback/db_thread.h"
#include "percona_playback/plugin.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <assert.h>
#include <unistd.h>

Dispatcher g_dispatcher;

extern percona_playback::DBClientPlugin *g_dbclient_plugin;

void
Dispatcher::db_thread_func(DBThread *db_thread)
{
  ++active_threads;
  db_thread->run();
  delete db_thread;
  --active_threads;
}

void
Dispatcher::start_thread(DBThread *db_thread)
{
  boost::thread(boost::bind(&Dispatcher::db_thread_func, this, db_thread));
}

boost::shared_ptr<DBThreadState>
Dispatcher::get_thread_state(
  uint64_t thread_id,
  boost::function1<void, DBThread *> run_on_db_thread_create)
{
  DBExecutorsTable::accessor a;

  if (executors.insert(a, thread_id))
  {
    DBThread *db_thread= g_dbclient_plugin->create(thread_id);
    assert(db_thread);
    a->second= db_thread;
    if (!run_on_db_thread_create.empty())
      run_on_db_thread_create(db_thread);
    start_thread(db_thread);
  }

  return a->second->get_state();
}

void
Dispatcher::dispatch(QueryEntryPtr query_entry)
{
  uint64_t thread_id= query_entry->getThreadId();
  {
    DBExecutorsTable::accessor a;
    if (executors.insert(a, thread_id))
    {
      DBThread *db_thread= g_dbclient_plugin->create(thread_id);
      a->second= db_thread;
      start_thread(db_thread);
    }
    a->second->queries.push(query_entry);
  }
}

bool
Dispatcher::finish(uint64_t thread_id)
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

  db_thread->queries.push(QueryEntryPtr(new FinishEntry()));
  return true;
}

void
Dispatcher::finish_all_and_wait()
{
  QueryEntryPtr shutdown_command(new FinishEntry());
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

    t->queries.push(shutdown_command);
  }

  while (active_threads != 0)
    usleep(100000);

}
