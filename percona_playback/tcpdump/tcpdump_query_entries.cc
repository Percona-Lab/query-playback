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

#include <config.h>
#include "tcpdump_query_entries.h"
#include "tcpdump.h"
#include "percona_playback/db_thread.h"
#include "percona_playback/plugin.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <string.h>
#include <iostream>
#include <assert.h>

void
TcpdumpQueryEntry::execute(DBThread *t)
{
  QueryResult expected_result;
  timeval start_time;
  timeval end_time;

  assert(t);

  ConnectionState &state= *connection_state;

  LastExecutedQueryInfo &last_query_info= state.last_executed_query_info;

  if (g_tcpdump_mode == TCPDUMP_MODE_ACCURATE &&
      last_query_info.end_timestamp.tv_sec != 0 &&
      last_query_info.end_timestamp.tv_usec != 0 &&
      last_query_info.end_pcap_timestamp.tv_sec != 0 &&
      last_query_info.end_pcap_timestamp.tv_usec != 0)
  {
    timeval current_timestamp;
    timeval pause_time;
    timeval pcap_pause_time;

    gettimeofday(&current_timestamp, NULL);
    timersub(&current_timestamp, &last_query_info.end_timestamp, &pause_time);

    timersub(&pcap_timestamp,
             &last_query_info.end_pcap_timestamp,
             &pcap_pause_time);

    if (timercmp(&pause_time, &pcap_pause_time, <))
    {
      timeval time_to_sleep;
      timersub(&pcap_pause_time,
               &pause_time,
               &time_to_sleep);
      usleep(time_to_sleep.tv_sec*1000000 + time_to_sleep.tv_usec);
    }
  }

  last_query_info.result.clear();

  gettimeofday(&start_time, NULL);
  t->execute_query(query, &last_query_info.result, expected_result);
  gettimeofday(&end_time, NULL);

  last_query_info.result.setDuration(
      boost::posix_time::microseconds(
        end_time.tv_sec*1000000 + end_time.tv_usec -
        start_time.tv_sec*1000000 - start_time.tv_usec));
  last_query_info.begin_pcap_timestamp= pcap_timestamp;
  last_query_info.end_timestamp= end_time;
  last_query_info.query= query;
}

void
TcpdumpResultEntry::execute(DBThread *t)
{
  timeval query_execution_time;
  timeval current_time;
  timeval tv_btw_query_end_and_result_start;
  boost::posix_time::time_duration total_duration;

  boost::shared_ptr<DBThreadState> thr_state;

  assert(t);

  ConnectionState &state= *connection_state;
  LastExecutedQueryInfo &last_query_info= state.last_executed_query_info;

  timersub(&pcap_timestamp,
           &last_query_info.begin_pcap_timestamp,
           &query_execution_time);

  expected_result.setDuration(
    boost::posix_time::microseconds(
      query_execution_time.tv_usec +
      query_execution_time.tv_sec*1000000));

  gettimeofday(&current_time, NULL);
  timersub(&current_time,
           &last_query_info.end_timestamp,
           &tv_btw_query_end_and_result_start);

  total_duration=
    last_query_info.result.getDuration() +
      boost::posix_time::microseconds(
        tv_btw_query_end_and_result_start.tv_sec*1000000 +
        tv_btw_query_end_and_result_start.tv_usec);

  if (g_tcpdump_mode == TCPDUMP_MODE_ACCURATE &&
      (expected_result.getDuration() > total_duration))
  {
    boost::posix_time::time_duration us_sleep_time=
      expected_result.getDuration() - total_duration;

    usleep(us_sleep_time.total_microseconds());
  }

  gettimeofday(&last_query_info.end_timestamp, NULL);
  last_query_info.end_pcap_timestamp= pcap_timestamp;

  BOOST_FOREACH(const percona_playback::PluginRegistry::ReportPluginPair pp,
                percona_playback::PluginRegistry::singleton().report_plugins)
  {
    pp.second->query_execution(getThreadId(),
                               last_query_info.query,
                               expected_result,
                               last_query_info.result);
  }
}
