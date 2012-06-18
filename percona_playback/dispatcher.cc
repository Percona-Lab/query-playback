#include "percona_playback/dispatcher.h"
#include "percona_playback/db_thread.h"
#include "percona_playback/plugin.h"

#include <assert.h>

Dispatcher g_dispatcher;

extern percona_playback::DBClientPlugin *g_dbclient_plugin;

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
    db_thread->start_thread();
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
      db_thread->start_thread();
    }
    a->second->queries.push(query_entry);
  }
}

bool
Dispatcher::finish_and_wait(uint64_t thread_id)
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
  db_thread->join();

  delete db_thread;

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
    t->join();

    delete t;
  }
}
