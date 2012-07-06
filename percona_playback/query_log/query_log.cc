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
#include <boost/thread.hpp>
#include "query_log.h"
#include <unistd.h>

#include <tbb/pipeline.h>
#include <tbb/tick_count.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/tbb_allocator.h>
#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_hash_map.h>

#include <percona_playback/percona_playback.h>
#include <percona_playback/plugin.h>
#include <percona_playback/db_thread.h>
#include <percona_playback/query_log/query_log.h>
#include <percona_playback/query_result.h>
#include "percona_playback/dispatcher.h"
#include <percona_playback/gettext.h>

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace po= boost::program_options;

static bool g_run_set_timestamp;
static bool g_preserve_query_time;

class ParseQueryLogFunc: public tbb::filter {
public:
  ParseQueryLogFunc(FILE *input_file_,
		    unsigned int run_count_,
		    tbb::atomic<uint64_t> *entries_,
		    tbb::atomic<uint64_t> *queries_)
    : tbb::filter(true),
      nr_entries(entries_),
      nr_queries(queries_),
      input_file(input_file_),
      run_count(run_count_)
  {};

  void* operator() (void*);
private:
  tbb::atomic<uint64_t> *nr_entries;
  tbb::atomic<uint64_t> *nr_queries;
  FILE *input_file;
  unsigned int run_count;
};

void* dispatch(void *input_);

void* ParseQueryLogFunc::operator() (void*)  {
  std::vector<boost::shared_ptr<QueryLogEntry> > *entries=
    new std::vector<boost::shared_ptr<QueryLogEntry> >();

  boost::shared_ptr<QueryLogEntry> tmp_entry(new QueryLogEntry());
  entries->push_back(tmp_entry);

  char *line= NULL;
  size_t buflen = 0;
  ssize_t len;

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

    if ((p[0] != '#' && (q-p) >= (ssize_t)strlen("started with:\n"))
	&& strncmp(q- strlen("started with:\n"), "started with:", strlen("started with:"))==0)
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
      tmp_entry.reset(new QueryLogEntry());
      entries->push_back(tmp_entry);
      (*this->nr_entries)++;
    }

    tmp_entry->add_line(std::string(line), nr_queries);
  next:
    if (count > 100)
    {
      count= 0;
      //      fseek(input_file,-len, SEEK_CUR);
      break;
    }
    if ((len= getline(&line, &buflen, input_file)) == -1)
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

  for ( it=query.begin() ; it < query.end(); ++it )
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

    boost::posix_time::time_duration expected_duration=
      boost::posix_time::microseconds(long(query_time * 1000000));
    expected_result.setDuration(expected_duration);

    boost::posix_time::ptime start_time;
    start_time= boost::posix_time::microsec_clock::universal_time();
    t->execute_query(*it, &r, expected_result);

    boost::posix_time::ptime end_time;
    end_time= boost::posix_time::microsec_clock::universal_time();

    boost::posix_time::time_period duration(start_time, end_time);
    r.setDuration(duration.length());

    if (g_preserve_query_time
        && expected_duration > duration.length())
    {
      boost::posix_time::time_duration us_sleep_time=
        expected_duration - duration.length();

      usleep(us_sleep_time.total_microseconds());
    }

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

void QueryLogEntry::add_line(const std::string &s, tbb::atomic<uint64_t> *nr_queries)
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

  {
    std::string qt_str("Query_time: ");
    size_t location= s.find(qt_str);
    if (location != std::string::npos)
    {
      query_time= strtod(s.c_str() + location + qt_str.length(), NULL);
    }
  }

  if (s[0] == '#' && strncmp(s.c_str(), "# administrator", strlen("# administrator")))
    info.push_back(s);
  else
  {
    query.push_back(s);
    (*nr_queries)++;
  }
}

extern percona_playback::DBClientPlugin *g_dbclient_plugin;

void* dispatch (void *input_)
{
    std::vector<boost::shared_ptr<QueryLogEntry> > *input= 
      static_cast<std::vector<boost::shared_ptr<QueryLogEntry> >*>(input_);
    for (unsigned int i=0; i< input->size(); i++)
    {
      //      usleep(10);
      g_dispatcher.dispatch((*input)[i]);
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

static void LogReaderThread(FILE* input_file, unsigned int run_count, struct percona_playback_run_result *r)
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

  g_dispatcher.finish_all_and_wait();

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

class QueryLogPlugin : public percona_playback::InputPlugin
{
private:
  po::options_description     options;
  std::string                 file_name;
  unsigned int                read_count;

public:
  QueryLogPlugin(const std::string &_name) :
    InputPlugin(_name),
    options(_("Query Log Options")),
    read_count(1)
  {};

  virtual boost::program_options::options_description* getProgramOptions() {
      options.add_options()
      ("query-log-file",
       po::value<std::string>(), _("Query log file"))
/* Disabled for 0.3 until we have something more universal.
      ("query-log-read-count",
       po::value<unsigned int>(&read_count)->default_value(1),
       _("Query log file read count (how many times to read query log file)"))
*/
      ("query-log-set-timestamp",
       po::value<bool>(&g_run_set_timestamp)->
          default_value(false)->
            zero_tokens(), 
       _("By default, we skip the SET TIMESTAMP=XX; query that the MySQL slow "
       "query log always includes. This may cause some subsequent queries to "
       "fail, depending on your workload. If the --run-set-timestamp option "
       "is enabled, we run these queries too."))
      ("query-log-preserve-query-time",
       po::value<bool>(&g_preserve_query_time)->
        default_value(false)->
          zero_tokens(),
       _("Ensure that each query takes at least Query_time (from slow query "
	 "log) to execute."))
      ;

    return &options;
  }

  virtual int processOptions(boost::program_options::variables_map &vm)
  {
    if (!active &&
        (vm.count("query-log-file") ||
//         !vm["query-log-read-count"].defaulted() ||
         !vm["query-log-preserve-query-time"].defaulted() ||
         !vm["query-log-set-timestamp"].defaulted()))
    {
      fprintf(stderr,_(("query-log plugin is not selected, "
			"you shouldn't use this plugin-related "
			"command line options\n")));
      return -1;
    }

    if (!active)
      return 0;

    if (vm.count("query-log-file"))
      file_name= vm["query-log-file"].as<std::string>();
    else
    {
      fprintf(stderr, _("ERROR: --query-log-file is a required option.\n"));
      return -1;
    }

    return 0;
  }

  virtual void run(percona_playback_run_result  &result)
  {
    run_query_log(file_name, read_count, &result);
  }
};

static void init(percona_playback::PluginRegistry&r)
{
  r.add("query-log", new QueryLogPlugin("query-log"));
}

PERCONA_PLAYBACK_PLUGIN(init);
