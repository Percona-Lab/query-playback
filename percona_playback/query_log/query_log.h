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

#ifndef PERCONA_PLAYBACK_QUERY_LOG_H
#define PERCONA_PLAYBACK_QUERY_LOG_H

#include <percona_playback/visibility.h>
#include "percona_playback/query_entry.h"
#include <iostream>
#include <string>
#include <deque>

#include <boost/chrono.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/make_shared.hpp>

PERCONA_PLAYBACK_API
int run_query_log(const std::string &log_file, unsigned int read_count, struct percona_playback_run_result *r);

#ifdef __cplusplus
extern "C"
{
#endif

class DBThread;

class QueryLogEntry : public QueryEntry
{
public:
  typedef boost::chrono::duration<int64_t, boost::nano> Duration;
  typedef boost::chrono::system_clock::time_point TimePoint;

private:
  boost::string_ref data; // query including metadata
  TimePoint start_time; // only valid if g_preserve_query_starttime is enabled

public:
  QueryLogEntry(boost::string_ref data, TimePoint end_time)
    : data(data), start_time(end_time - boost::chrono::microseconds((long)(parseQueryTime() * boost::micro::den))) {
  }

  virtual uint64_t getThreadId() const {
    return parseThreadId();
  }

  virtual void execute(DBThread *t);

  virtual bool is_quit() const;

  uint64_t parseThreadId() const;
  uint64_t parseRowsSent() const;
  uint64_t parseRowsExamined() const;
  double parseQueryTime() const;

  // only valid if g_preserve_query_starttime is enabled
  TimePoint getStartTime() const;

  std::string getQuery(bool remove_timestamp);

  void display()
  {
    std::cerr << "    " << getQuery(true) << std::endl;
  }

  bool operator <(const QueryLogEntry& second) const;
};


class QueryLogEntries : public QueryEntries {
public:
  typedef std::deque<QueryLogEntry> Entries;
  Entries entries;

  boost::shared_ptr<QueryEntry> popEntry() {
    boost::shared_ptr<QueryEntry> entry;
    if (!entries.empty()) {
      entry = boost::make_shared<QueryLogEntry>(entries.front());
      entries.pop_front();
    }
    return entry;
  }

  virtual void setShutdownOnLastQueryOfConn();
};

#ifdef __cplusplus
}
#endif

#endif /* PERCONA_PLAYBACK_QUERY_LOG_H */
