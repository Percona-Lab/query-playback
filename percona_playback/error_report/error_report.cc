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

#include <cstdio>
#include <cstdlib>
#include <iostream>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>

#include <tbb/tbb_stddef.h>

/* We're all conditional here because TBB in CentOS 6 is old */
#if TBB_VERSION_MAJOR < 3
#include <boost/unordered_map.hpp>
#include <tbb/mutex.h>
#else
#include <tbb/concurrent_unordered_map.h>
#endif

#include <tbb/atomic.h>
#include <percona_playback/plugin.h>
#include <percona_playback/query_result.h>
#include <percona_playback/gettext.h>

class ErrorReportPlugin : public percona_playback::ReportPlugin
{
private:
  tbb::atomic<uint64_t> total_execution_time_ms;
  tbb::atomic<uint64_t> expected_total_execution_time_ms;
  tbb::atomic<uint64_t> nr_quicker_queries;
  std::string  full_report;

#if TBB_VERSION_MAJOR < 3
  tbb::mutex connection_query_count_mutex;
  typedef boost::unordered_map<uint64_t, tbb::atomic<uint64_t> > ConnectionQueryCountMap;
#else
  typedef tbb::concurrent_unordered_map<uint64_t, tbb::atomic<uint64_t> > ConnectionQueryCountMap;
#endif

  typedef std::pair<uint64_t, tbb::atomic<uint64_t> > ConnectionQueryCountPair;
  typedef std::map<uint64_t, uint64_t> SortedConnectionQueryCountMap;
  typedef std::pair<uint64_t, uint64_t> SortedConnectionQueryCountPair;

  ConnectionQueryCountMap connection_query_counts;


public:
  ErrorReportPlugin(std::string _name) : ReportPlugin(_name)
  {
    total_execution_time_ms= 0;
    expected_total_execution_time_ms= 0;
    nr_quicker_queries= 0;
    full_report = "";
  }

  virtual void query_execution(const uint64_t thread_id,
			       const std::string &query,
			       const QueryResult &expected,
			       const QueryResult &actual)
  {
  //    fprintf(stderr,_("Error query: %s\n"), query.c_str());

    total_execution_time_ms.fetch_and_add(actual.getDuration().total_microseconds());

    if (expected.getDuration().total_microseconds())
    {
      expected_total_execution_time_ms.fetch_and_add(expected.getDuration().total_microseconds());
      if (actual.getDuration().total_microseconds() > expected.getDuration().total_microseconds())
      {
        printf(_("thread %" PRIu64 " slower query was run in  %" PRIu64 " microseconds instead of %" PRIu64  "\n  <--\n%s  -->\n"), 
		uint64_t(thread_id),
		uint64_t(actual.getDuration().total_microseconds()),
                uint64_t(expected.getDuration().total_microseconds()),
		query.c_str());
      }
    }
  }

 virtual void print_report()
 {
    printf(_("Error Report finished\n\n\n"));
 }


};

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("error_report", new ErrorReportPlugin("error_report"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
