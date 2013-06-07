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
#include <percona_playback/gettext.h>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/crc.hpp>
#include <vector>

#include <tbb/concurrent_hash_map.h>
#include <stdio.h>

extern percona_playback::DBClientPlugin *g_dbclient_plugin;

class ThreadPoolDispatcher :
	public percona_playback::DispatcherPlugin
{
  typedef std::vector<boost::shared_ptr<DBThread> > Workers;

  unsigned			threads_count;
  boost::
    program_options::
      options_description	options;
  Workers			workers;

public:
  ThreadPoolDispatcher(std::string _name) :
	  DispatcherPlugin(_name),
	  threads_count(0),
	  options("Threads-pool Options") {}

  virtual void dispatch(QueryEntryPtr query_entry);
  virtual bool finish_and_wait(uint64_t) { return true; }
  virtual void finish_all_and_wait();
  virtual void run();

  boost::program_options::options_description* getProgramOptions();
  int processOptions(boost::program_options::variables_map &vm);

};

void ThreadPoolDispatcher::run()
{
  for (unsigned i = 0; i < threads_count; ++i)
  {
    boost::shared_ptr<DBThread> db_thread(g_dbclient_plugin->create(i));
    workers.push_back(db_thread);
    db_thread->start_thread();
  }
}

void ThreadPoolDispatcher::dispatch(QueryEntryPtr query_entry)
{
  /*
    Each worker has its own queue. For some types of input plugins
    it is important to execute query entries with the same thread id
    by the same worker. That is why we choose worker by simple hash from
    thread id.
  */
  uint64_t thread_id= query_entry->getThreadId();
  boost::crc_32_type crc;
  crc.process_bytes(&thread_id, sizeof(thread_id));
  uint32_t worker_index = crc.checksum() % workers.size();
  workers[worker_index]->queries->push(query_entry);
}

void ThreadPoolDispatcher::finish_all_and_wait()
{
  QueryEntryPtr shutdown_command(new FinishEntry(0));
  for (Workers::iterator i = workers.begin(), end = workers.end(); i != end; ++i)
    (*i)->queries->push(shutdown_command);
  for (Workers::iterator i = workers.begin(), end = workers.end(); i != end; ++i)
    (*i)->join();
  workers.clear();
}


boost::program_options::options_description*
ThreadPoolDispatcher::getProgramOptions()
{
  unsigned default_threads_count =
    boost::thread::hardware_concurrency();
  options.add_options()
     ("thread-pool-threads-count",
     boost::program_options::value<unsigned>(&threads_count)
       ->default_value(default_threads_count),
     _("The number of threads in thread pool. If this options is omitted "
       "the number of threads equals to hardware concurency."))
    ;

  return &options;

}

int
ThreadPoolDispatcher::processOptions(boost::program_options::variables_map &vm)
{
  if (!active &&
      !vm["thread-pool-threads-count"].defaulted())
  {
    fprintf(stderr, _("thread-pool plugin is not selected, "
                      "you shouldn't use this plugin-related "
                      "command line options\n"));
    return -1;
  }

  return 0;
}

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("thread-pool",
	new ThreadPoolDispatcher("thread-pool"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
