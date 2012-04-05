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

#include <cstdio>
#include <cstdlib>
#include <iostream>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "tbb/atomic.h"
#include <percona_playback/plugin.h>
#include <percona_playback/query_result.h>

class SimpleReportPlugin : public percona_playback::ReportPlugin
{
private:
  tbb::atomic<uint64_t> nr_expected_rows_sent;
  tbb::atomic<uint64_t> nr_actual_rows_sent;
  tbb::atomic<uint64_t> nr_queries_rows_differ;
  tbb::atomic<uint64_t> nr_queries_executed;
  tbb::atomic<uint64_t> nr_error_queries;
public:
  SimpleReportPlugin(std::string _name) : ReportPlugin(_name)
  {
    nr_queries_executed= 0;
    nr_expected_rows_sent= 0;
    nr_actual_rows_sent= 0;
    nr_queries_rows_differ= 0;
    nr_error_queries= 0;
  }

  virtual void query_execution(const uint64_t thread_id,
			       const std::string &query,
			       const QueryResult &expected,
			       const QueryResult &actual)
  {
    if (actual.getError())
      nr_error_queries++;

    if (actual.getRowsSent() != expected.getRowsSent())
    {
      nr_queries_rows_differ++;
      std::cerr << "Rows sent: " << actual.getRowsSent()
		<< " != expected " << expected.getRowsSent()
		<< std::endl;
    }
    nr_queries_executed++;
    nr_expected_rows_sent.fetch_and_add(expected.getRowsSent());
    nr_actual_rows_sent.fetch_and_add(actual.getRowsSent());
  }

  virtual void print_report()
  {
    printf("Report\n");
    printf("------\n");
    printf("Executed %" PRIu64 " queries\n", uint64_t(nr_queries_executed));
    printf("A total of %" PRIu64 " queries had errors.\n",
	   uint64_t(nr_error_queries));
    printf("Expected %" PRIu64 " rows, got %" PRIu64 " (a difference of %" PRId64 ")\n",
	   uint64_t(nr_expected_rows_sent),
	   uint64_t(nr_actual_rows_sent),
	   labs(int64_t(nr_expected_rows_sent) - int64_t(nr_actual_rows_sent))
	   );
    printf("Number of queries where number of rows differed: %" PRIu64 ".\n",
	   uint64_t(nr_queries_rows_differ));
  }

};

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("simple_report", new SimpleReportPlugin("simple_report"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
