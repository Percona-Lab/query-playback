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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef __APPLE__
        #include <sys/uio.h>
#else
        #include <sys/io.h>
#endif
#include <sys/mman.h>

#include <tbb/atomic.h>

#include <percona_playback/percona_playback.h>
#include <percona_playback/plugin.h>
#include <percona_playback/db_thread.h>
#include <percona_playback/query_log/query_log.h>
#include <percona_playback/query_result.h>

#include <boost/atomic.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/make_shared.hpp>
#include <boost/unordered_set.hpp>

namespace po= boost::program_options;

static bool g_run_set_timestamp;
static bool g_preserve_query_time;
static bool g_accurate_mode;
static bool g_disable_sorting;
static bool g_use_innodb_trx_id;

static boost::atomic<long long> g_max_behind_ns;

extern percona_playback::DispatcherPlugin *g_dispatcher_plugin;

// workaround bug in boost 1.53 string_ref::find(string_ref)
static boost::string_ref::size_type find(boost::string_ref str, boost::string_ref substr) {
  boost::string_ref::const_iterator iter;
  iter = std::search(str.cbegin(), str.cend(), substr.cbegin(), substr.cend());
  return iter == str.cend() ? boost::string_ref::npos : std::distance(str.cbegin(), iter);
}

// returns next line starting from current position inside the data and updating it.
// a empty string signals that the end is reached.
static boost::string_ref readline(boost::string_ref data, boost::string_ref::size_type& pos) {
  boost::string_ref line;
  if (pos == boost::string_ref::npos || pos >= data.size())
    return line;
  boost::string_ref::size_type new_pos = data.substr(pos).find('\n');
  if (new_pos != boost::string_ref::npos)
    ++new_pos;
  line = data.substr(pos, new_pos);
  pos = (new_pos == boost::string_ref::npos) ? new_pos : pos + new_pos;
  return line;
}

// removes the spefified chars from the string from the left and right side
static boost::string_ref trim(boost::string_ref str, boost::string_ref chars = " \n\r\t") {
  boost::string_ref::size_type start_pos = str.find_first_not_of(chars);
  if (start_pos == boost::string_ref::npos)
    return boost::string_ref();
  return str.substr(start_pos, str.find_last_not_of(chars) + 1 - start_pos);
}

static bool parse_time(boost::string_ref s, QueryLogData::TimePoint& start_time) {
  // Time can look like this
  //   # Time: 090402 9:23:36
  // or like this if microseconds granularity is configured.
  //   # Time: 090402 9:23:36.123456
  long long msecs = 0;
  std::tm td;
  memset(&td, 0, sizeof(td));
  std::string line(s.begin(), s.end());
  int num_read = sscanf(line.c_str(), "# Time: %02d%02d%02d %2d:%02d:%02d.%06lld",
                        &td.tm_year, &td.tm_mon, &td.tm_mday, &td.tm_hour, &td.tm_min, &td.tm_sec, &msecs);
  if (num_read < 6)
    return false;

  // months [0, 11]
  td.tm_mon -= 1;
  // years since 1900
  td.tm_year += td.tm_year < 70 ? 100 : 0;
  start_time = boost::chrono::system_clock::from_time_t(std::mktime(&td));
  start_time += boost::chrono::microseconds(msecs);

  return true;
}

boost::shared_ptr<QueryLogEntries> getEntries(boost::string_ref data)  {
  boost::shared_ptr<QueryLogEntries> entries = boost::make_shared<QueryLogEntries>();

  QueryLogData::TimePoint current_timestamp;
  boost::string_ref::size_type pos = 0;

  boost::string_ref line, next_line;
  for (;;) {
    if (next_line.empty()) {
      line = readline(data, pos);
      if (line.empty())
        break;
    } else {
      line = next_line;
      next_line.clear();
    }

    if (!line.starts_with('#')) {
      // skip lines like: "/usr/sbin/mysqld, ... started with:"
      if (line.ends_with("started with:\n"))
        continue;

      // skip lines like: "Tcp port: 3306 Unix socket: /var/lib/mysql/mysql.sock"
      if (line.starts_with("Tcp port: "))
        continue;

      // skip lines like: "Time[ ]+Id[ ]+Command[ ]+Argument"
      if (line.starts_with("Time "))
        continue;
    } else {
      /*
        # fixme, process admin commands.
        if (strcmp(p,"# administrator command: Prepare;\n") == 0)
        goto next;
      */

      // not every query has time metadata. Because only if the timestamp changes a new entry will get generated.
      // this is why we have a field inside the QueryLogData to save the timestamp.
      if (line.starts_with("# Time")) {
        parse_time(line, current_timestamp);
        continue;
      }

      // read whole metadata (except '# Time') and query
      int num_sql_lines = 0;
      boost::string_ref::size_type query_data_len = 0;
      next_line = line;
      do {
        if (!next_line.starts_with('#'))
          ++num_sql_lines;
        query_data_len += next_line.size();
        next_line = readline(data, pos);

        // stop if we find a line starting with "# User@Host" or "# Time" because it signals start of new query
      } while (!next_line.empty() && (!next_line.starts_with("# User@Host") && !next_line.starts_with("# Time")));
      entries->setNumEntries(entries->getNumEntries() + 1);

      if (num_sql_lines) {
        if (g_accurate_mode && current_timestamp == QueryLogData::TimePoint()) {
          std::cerr << "WARNING: did not find a timestamp for the first query. Disabling accurate mode." << std::endl;
          g_accurate_mode = false;
        }

        // found a query, add it to the entries
        boost::string_ref query_data(line.data(), query_data_len);
        entries->entries.push_back(QueryLogData(query_data, current_timestamp));
        entries->setNumQueries(entries->getNumQueries() + 1);
      }
    }
  }

  if (!g_disable_sorting || g_accurate_mode)
    std::stable_sort(entries->entries.begin(), entries->entries.end());

  return entries;
}

bool QueryLogData::operator <(const QueryLogData& right) const {
  // for same connections we make sure that the follow the order in the query log
  if (parseThreadId() == right.parseThreadId())
    return data.data() < right.data.data();

  if (g_use_innodb_trx_id) {
    uint64_t id_left = parseInnoDBTrxId();
    uint64_t id_right = right.parseInnoDBTrxId();
    if (id_left && id_right)
      return id_left < id_right;
  }

  return getStartTime() < right.getStartTime();
}

void QueryLogData::execute(DBThread *t)
{
  std::string query = getQuery(!g_run_set_timestamp);

  QueryResult expected_result;
  expected_result.setRowsSent(parseRowsSent());
  expected_result.setRowsExamined(parseRowsExamined());
  expected_result.setError(0);

  boost::posix_time::time_duration expected_duration=
    boost::posix_time::microseconds(long(parseQueryTime() * 1000000));
  expected_result.setDuration(expected_duration);

  if (g_accurate_mode) {
    // UGLY: This requires that static var initalization is thread safe,
    //       which is true for c++11 and gcc, clang even before c++11 but not for MSVC.
    static Duration first_query_exec_offset = boost::chrono::system_clock::now() - getStartTime();

    Duration time_diff = getStartTime() + first_query_exec_offset - boost::chrono::system_clock::now();

    // update max time behind, it does this atomic:
    //  g_max_behind_ns = std::max(g_max_behind_ns, -time_diff.count())
    long long current_val = g_max_behind_ns;
    long long new_value = -time_diff.count();
    while (current_val < new_value &&
           !g_max_behind_ns.compare_exchange_weak(current_val, new_value)) {
      // nothing to do here - update is inside the while condition...
    }

    boost::this_thread::sleep_for(time_diff);
  }

  boost::posix_time::ptime start_time;
  start_time= boost::posix_time::microsec_clock::universal_time();

  QueryResult r;
  t->execute_query(query, &r, expected_result);

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

  uint64_t thread_id = parseThreadId();
  BOOST_FOREACH(const percona_playback::PluginRegistry::ReportPluginPair pp,
                percona_playback::PluginRegistry::singleton().report_plugins)
  {
    if (pp.second->active)
      pp.second->query_execution(thread_id,
                                 query,
                                 expected_result,
                                 r);
  }
}

bool QueryLogData::is_quit() const {
  return find(data, "\n# administrator command: Quit;") != boost::string_ref::npos;
}

std::string QueryLogData::getQuery(bool remove_timestamp) {
  std::string ret;
  boost::string_ref::size_type pos = 0;
  bool found_non_comment_line = false;
  for (boost::string_ref line = readline(data, pos); !line.empty(); line = readline(data, pos)) {
    // we ignore the metadata but also make sure that we don't remove "# administrator command " strings
    if (!found_non_comment_line && line.starts_with('#'))
      continue;
    found_non_comment_line = true;
    if (remove_timestamp && line.starts_with("SET timestamp="))
      continue;
    boost::string_ref trimmed_line = trim(line);
    if (trimmed_line.empty())
      continue;
    if (!ret.empty())
      ret.push_back(' ');
    ret.append(trimmed_line.begin(), trimmed_line.end());
  }
  return ret;
}

uint64_t QueryLogData::parseThreadId() const {
  // use cached thread id
  if (thread_id)
    return thread_id;

  size_t location= find(data, "Thread_id: ");
  if (location != std::string::npos) {
    thread_id = strtoull(&data[location + strlen("Thread_Id: ")], NULL, 10);
    return thread_id;
  }

  // starting from MySQL 5.6.2 (bug #53630) the thread id is included as "Id:"
  location= find(data, "Id: ");
  if (location != std::string::npos) {
     thread_id = strtoull(&data[location + strlen("Id: ")], NULL, 10);
     return thread_id;
  }
  return 0;
}

uint64_t QueryLogData::parseRowsSent() const {
  size_t location= find(data, "Rows_sent: ");
  if (location != std::string::npos)
    return strtoull(&data[location + strlen("Rows_sent: ")], NULL, 10);
  return 0;
}

uint64_t QueryLogData::parseRowsExamined() const {
  size_t location= find(data, "Rows_Examined: ");
  if (location != std::string::npos)
    return strtoull(&data[location + strlen("Rows_examined: ")], NULL, 10);
  return 0;
}

double QueryLogData::parseQueryTime() const {
  size_t location= find(data, "Query_time: ");
  if (location != std::string::npos)
    return strtod(&data[location + strlen("Query_time: ")], NULL);
  return 0.0;
}

uint64_t QueryLogData::parseInnoDBTrxId() const {
  // use cached innodb trx id
  if (innodb_trx_id != -1)
    return innodb_trx_id;

  innodb_trx_id = 0;
  size_t location= find(data, "InnoDB_trx_id: ");
  if (location != std::string::npos) {
    innodb_trx_id = strtoull(&data[location + strlen("InnoDB_trx_id: ")], NULL, 16 /* hex number */);
  }
  return innodb_trx_id;
}

extern percona_playback::DBClientPlugin *g_dbclient_plugin;

static void LogReaderThread(boost::string_ref data, struct percona_playback_run_result *r)
{
  boost::shared_ptr<QueryLogEntries> entry_vec = getEntries(data);
  g_dispatcher_plugin->dispatch(entry_vec);

  g_dispatcher_plugin->finish_all_and_wait();

  r->n_log_entries= entry_vec->getNumEntries();
  r->n_queries= entry_vec->getNumQueries();
}

void QueryLogEntries::setShutdownOnLastQueryOfConn() {
  // automatically close threads after last request
  boost::unordered_set<uint64_t> thread_ids;
  for (Entries::reverse_iterator it = entries.rbegin(), end = entries.rend(); it != end; ++it) {
    if (thread_ids.insert(it->parseThreadId()).second)
      last_query_of_conn.insert(it->data.data());
  }
}

class QueryLogPlugin : public percona_playback::InputPlugin
{
private:
  po::options_description     options;
  std::string                 file_name;
  unsigned int                read_count;
  bool			      std_in;

public:
  QueryLogPlugin(const std::string &_name) :
    InputPlugin(_name),
    options(_("Query Log Options")),
    read_count(1),
    std_in(false)
  {};

  virtual boost::program_options::options_description* getProgramOptions() {
      options.add_options()
      ("query-log-file",
       po::value<std::string>(), _("Query log file"))
      ("query-log-stdin",
       po::value<bool>()->default_value(false)->zero_tokens(),
       _("Read query log from stdin"))
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
      ("query-log-accurate-mode",
       po::value<bool>(&g_accurate_mode)->
        default_value(false)->
          zero_tokens(),
       _("Preserves pauses between queries."))
      ("query-log-disable-sorting",
       po::value<bool>(&g_disable_sorting)->
        default_value(false)->
          zero_tokens(),
       _("Disables the sorting of queries based on time (and InnoDB TRX ID). "
         "Instead replays queries in the order they appear in the log. "
         "Ignored in accurate mode which always does the sorting."))
      ("query-log-use-innodb-trx-id",
       po::value<bool>(&g_use_innodb_trx_id)->
        default_value(true),
       _("Uses the InnoDB Transaction Id to sort queries for improved accuracy. (Default: on)"))
      ;

    return &options;
  }

  virtual int processOptions(boost::program_options::variables_map &vm)
  {
    if (!active &&
        (vm.count("query-log-file") ||
	 !vm["query-log-stdin"].defaulted() ||
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

    if (vm.count("query-log-file") && vm["query-log-stdin"].as<bool>())
    {
      fprintf(stderr,  _(("The options --query-log-file and --query-log-stdin "
			  "can not be used together\n")));
      return -1;
    }

    if (vm.count("query-log-file"))
      file_name= vm["query-log-file"].as<std::string>();
    else if (vm["query-log-stdin"].as<bool>())
    {
      std_in = true;
    }
    else
    {
      fprintf(stderr, _("ERROR: --query-log-file is a required option.\n"));
      return -1;
    }

    return 0;
  }

  virtual void run(percona_playback_run_result &result)
  {
    if (std_in)
    {
      // TODO: maybe we want to create a temp file in order to safe RAM...
      const int block_size = 1024*32;
      std::string data;
      while (true) {
        std::string::size_type old_size = data.size();
        data.resize(old_size + block_size);
        int num_read = fread(&data[old_size], 1, block_size, stdin);
        if (num_read < block_size) {
          data.resize(old_size + num_read);
          break;
        }
      }
      boost::thread log_reader_thread(LogReaderThread, data, &result);
      log_reader_thread.join();
    }
    else
    {
      // read only mmap slowlog
      struct stat s;
      int fd = open(file_name.c_str(), O_RDONLY);
      if (fd == -1 || fstat(fd, &s) == -1) {
        fprintf(stderr,
          _("ERROR: Error opening file '%s': %s\n"),
          file_name.c_str(), strerror(errno));
        return;
      }
      boost::string_ref data;
      off_t size = s.st_size;
      void* ptr = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
      if (ptr == MAP_FAILED) {
        fprintf(stderr,
          _("ERROR: Error mmaping file '%s': %s\n"),
          file_name.c_str(), strerror(errno));
        return;
      }
      data = boost::string_ref(static_cast<const char*>(ptr), size);
      boost::thread log_reader_thread(LogReaderThread, data, &result);
      log_reader_thread.join();

      munmap(const_cast<char*>(data.data()), data.size());
      close(fd);
    }
  }
};

class QueryLogReportPlugin : public percona_playback::ReportPlugin {
public:
  QueryLogReportPlugin(std::string _name) : percona_playback::ReportPlugin(_name) {}
  virtual void query_execution(const uint64_t thread_id,
                               const std::string &query,
                               const QueryResult &expected,
                               const QueryResult &actual) {}

  virtual void print_report() {
    if (!g_accurate_mode)
      return;

    typedef boost::chrono::duration<int64_t, boost::milli> Milliseconds;

    std::cout << _("Query log report\n----------------\n");
    std::cout << _("Queries got delayed up to ");
    std::cout << boost::chrono::duration_cast<Milliseconds>(boost::chrono::nanoseconds(g_max_behind_ns));
    std::cout << _(" from the time they should have gotten executed. (smaller is better)");
    std::cout << std::endl << std::endl << std::endl;
  }

};

static void init(percona_playback::PluginRegistry&r)
{
  r.add("query-log", new QueryLogPlugin("query-log"));
  r.add("query-log-report", new QueryLogReportPlugin("query-log-report"));
}

PERCONA_PLAYBACK_PLUGIN(init);
