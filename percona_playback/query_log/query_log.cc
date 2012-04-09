/* BEGIN LICENSE
 * Copyright (C) 2011-2012 Percona Inc.
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

#include "config.h"

#include <stdlib.h>
#include <string>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <assert.h>
#include "boost/thread.hpp"
#include "query_log.h"
#include <unistd.h>

#include "tbb/pipeline.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tbb_allocator.h"
#include "tbb/atomic.h"
#include "tbb/concurrent_queue.h"
#include "tbb/concurrent_hash_map.h"

#include <percona_playback/percona_playback.h>
#include <percona_playback/plugin.h>
#include <percona_playback/db_thread.h>
#include <percona_playback/mysql_client/mysql_client.h>
#include <percona_playback/query_log/query_log.h>
#include <percona_playback/query_result.h>

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace po= boost::program_options;

static bool g_run_set_timestamp;

class ParseQueryLogFunc: public tbb::filter {
public:
  ParseQueryLogFunc(FILE *input_file_,
		    unsigned int run_count_,
		    tbb::atomic<uint64_t> *entries_,
		    tbb::atomic<uint64_t> *queries_)
    : entries(entries_),
      queries(queries_),
      input_file(input_file_),
      run_count(run_count_),
      tbb::filter(true)
  {};

  void* operator() (void*);
private:
  tbb::atomic<uint64_t> *entries;
  tbb::atomic<uint64_t> *queries;
  FILE *input_file;
  unsigned int run_count;
};

void* dispatch(void *input_);

void* ParseQueryLogFunc::operator() (void*)  {
  std::vector<QueryLogEntry> *entries = new std::vector<QueryLogEntry>();

  entries->push_back(QueryLogEntry());

  char *line= NULL;
  size_t buflen = 0;
  size_t len;

  if ((len= getline(&line, &buflen, input_file)) == -1)
  {
    delete entries;
    return NULL;
  }

  char *p= line;
  char *q;
  int count= 0;

  for (;;) {
    q= line+len;

    if ( (strncmp(p, "# Time", 5) == 0))
      goto next;

    if (p[0] != '#' && (q-p) >= strlen("started with:\n")
	&& strncmp(q- strlen("started with:\n"), "started with:\n", strlen("started with:\n"))==0)
      goto next;

    if (p[0] != '#' && strncmp(p, "Tcp port: ", strlen("Tcp port: "))==0)
      goto next;

    if (p[0] != '#' && strncmp(p, "Time    ", strlen("Time    "))==0)
      goto next;

    /*
      # fixme, process admin commands.
      if (strcmp(p,"# administrator command: Prepare;\n") == 0)
      goto next;
    */

    if (strncmp(p, "# User@Host", strlen("# User@Host")) == 0)
    {
      count++;
      entries->push_back(QueryLogEntry());
      (*this->entries)++;
    }

    entries->back().add_line(std::string(line), queries);
  next:
    if (count > 100)
    {
      count= 0;
      //      fseek(input_file,-len, SEEK_CUR);
      break;
    }
    if (getline(&line, &len, input_file) == -1)
    {
      break;
    }
    p= line;
  }

  free(line);
  return entries;
}

void QueryLogEntry::execute(DBThread *t)
{
  std::vector<std::string>::iterator it;
  QueryResult r;

  for ( it=query.begin() ; it < query.end(); it++ )
  {
    const std::string timestamp_query("SET timestamp=");
    if(!g_run_set_timestamp
       && (*it).compare(0, timestamp_query.length(), timestamp_query) == 0)
      continue;
    /*          std::cerr << "thread " << getThreadId()
		<< " running query " << (*it) << std::endl;*/

    QueryResult expected_result;
    expected_result.setRowsSent(rows_sent);
    expected_result.setRowsExamined(rows_examined);
    expected_result.setError(0);

    boost::posix_time::ptime start_time;
    start_time= boost::posix_time::microsec_clock::universal_time();
    t->execute_query(*it, &r, expected_result);

    boost::posix_time::ptime end_time;
    end_time= boost::posix_time::microsec_clock::universal_time();

    boost::posix_time::time_period duration(start_time, end_time);
    r.setDuration(duration.length());

    BOOST_FOREACH(const percona_playback::PluginRegistry::ReportPluginPair pp,
		  percona_playback::PluginRegistry::singleton().report_plugins)
    {
      pp.second->query_execution(getThreadId(),
				 (*it),
				 expected_result,
				 r);
    }
  }
}

void QueryLogEntry::add_line(const std::string &s, tbb::atomic<uint64_t> *queries)
{
  {
    size_t location= s.find("Thread_id: ");
    if (location != std::string::npos)
    {
      thread_id = strtoull(s.c_str() + location + strlen("Thread_Id: "), NULL, 10);
    }
  }

  {
    size_t location= s.find("Rows_sent: ");
    if (location != std::string::npos)
    {
      rows_sent = strtoull(s.c_str() + location + strlen("Rows_sent: "), NULL, 10);
    }
  }

  {
    size_t location= s.find("Rows_Examined: ");
    if (location != std::string::npos)
    {
      rows_sent = strtoull(s.c_str() + location + strlen("Rows_examined: "), NULL, 10);
    }
  }

  if (s[0] == '#' && strncmp(s.c_str(), "# administrator", strlen("# administrator")))
    info.push_back(s);
  else
  {
    query.push_back(s);
    (*queries)++;
  }
}

extern percona_playback::DBClientPlugin *g_dbclient_plugin;

void* dispatch (void *input_)
{
    std::vector<QueryLogEntry> *input= static_cast<std::vector<QueryLogEntry>*>(input_);
    for (int i=0; i< input->size(); i++)
    {
      //      usleep(10);
      uint64_t thread_id= (*input)[i].getThreadId();
      {
	DBExecutorsTable::accessor a;
	if (db_executors.insert( a, thread_id ))
	{
	  DBThread *db_thread= g_dbclient_plugin->create(thread_id);
	  a->second= db_thread;
	  db_thread->start_thread();
	}
	a->second->queries.push((*input)[i]);
      }
    }
    delete input;
    return NULL;
}

class DispatchQueriesFunc : public tbb::filter {
public:
  DispatchQueriesFunc() : tbb::filter(true) {};

  void* operator() (void *input_)
  {
    return dispatch(input_);
  }
};

void LogReaderThread(FILE* input_file, unsigned int run_count, struct percona_playback_run_result *r)
{
  tbb::pipeline p;
  tbb::atomic<uint64_t> entries;
  tbb::atomic<uint64_t> queries;
  entries=0;
  queries=0;

  ParseQueryLogFunc f2(input_file, run_count, &entries, &queries);
  DispatchQueriesFunc f4;
  p.add_filter(f2);
  p.add_filter(f4);
  p.run(2);

  QueryLogEntry shutdown_command;
  shutdown_command.set_shutdown();

  while(db_executors.size())
  {
    uint64_t thread_id;
    DBThread *t;
    {
      DBExecutorsTable::const_iterator iter= db_executors.begin();
      thread_id= (*iter).first;
      t= (*iter).second;
    }
    db_executors.erase(thread_id);
    t->queries.push(shutdown_command);
    t->join();
    delete t;
  }

  r->n_log_entries= entries;
  r->n_queries= queries;
}

int run_query_log(const std::string &log_file, unsigned int run_count, struct percona_playback_run_result *r)
{
  FILE* input_file = fopen(log_file.c_str(),"r");
  if (input_file == NULL)
    return -1;

  boost::thread log_reader_thread(LogReaderThread,input_file, run_count, r);

  log_reader_thread.join();
  fclose(input_file);

  return 0;
}

class QueryLogPlugin : public percona_playback::plugin
{
private:

public:
  QueryLogPlugin(std::string _name) {};

  virtual boost::program_options::options_description* getProgramOptions() {
    static po::options_description query_log_options("Query Log Options");
    query_log_options.add_options()
      ("slow-query-log-file", po::value<std::string>(), "Query log file")
      ("file-read-count", po::value<unsigned int>(), "Query log file read count (how many times to read query log file)")
      ("run-set-timestamp", po::value<bool>(&g_run_set_timestamp)->default_value(false)->zero_tokens(), "By default, we skip the SET TIMESTAMP=XX; query that the MySQL slow query log always includes. This may cause some subsequent queries to fail, depending on your workload. If the --run-set-timestamp option is enabled, we run these queries too.")
      ;

    return &query_log_options;
  }

  virtual int processOptions(boost::program_options::variables_map &vm) {
    // FIXME: move this from percona_playback.cc
    return 0;
  }

};

void init(percona_playback::PluginRegistry&r)
{
  r.add("query_log", new QueryLogPlugin("query_log"));
}

PERCONA_PLAYBACK_PLUGIN(init);
