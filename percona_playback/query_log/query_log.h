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
#include <boost/unordered_set.hpp>

PERCONA_PLAYBACK_API
int run_query_log(const std::string &log_file, unsigned int read_count, struct percona_playback_run_result *r);

#ifdef __cplusplus
extern "C"
{
#endif

class DBThread;

// This class represents a query inside the slowlog.
// Because we keep all query data in memory it's very important that the per query overhead is small.
// This is why we split QueryLogData from QueryLogEntry. We try to keep all queries as long as possible
// as QueryLogData and only create a QueryLogEntry when neccessary.
// This saves about 16 bytes per query (8 bytes (vptr) + 1 byte (shutdown bool) + 7 bytes alignment)
// -> total size per query is 32 bytes (on a 64bit system).
class QueryLogData {
public:
  typedef boost::chrono::duration<int64_t, boost::nano> Duration;
  typedef boost::chrono::system_clock::time_point TimePoint;

  boost::string_ref data; // query including metadata
  mutable uint64_t thread_id; // we cache the thread id
  TimePoint start_time;
  mutable uint64_t innodb_trx_id;

public:
  QueryLogData(boost::string_ref data, TimePoint end_time)
    : data(data), thread_id(0),
      start_time(end_time - boost::chrono::microseconds((long)(parseQueryTime() * boost::micro::den))),
      innodb_trx_id(-1) {
  }

  void execute(DBThread *t);
  bool is_quit() const;

  uint64_t parseThreadId() const;
  uint64_t parseRowsSent() const;
  uint64_t parseRowsExamined() const;
  double parseQueryTime() const;
  uint64_t parseInnoDBTrxId() const;

  TimePoint getStartTime() const { return start_time; }

  std::string getQuery(bool remove_timestamp);

  bool operator <(const QueryLogData& second) const;
};

class QueryLogEntry : public QueryEntry
{
private:
  QueryLogData data;

public:
  QueryLogEntry(QueryLogData data) : data(data) {}

  virtual uint64_t getThreadId() const { return data.parseThreadId(); }

  virtual void execute(DBThread *t) { data.execute(t); }

  virtual bool is_quit() const { return data.is_quit(); }

  std::string getQuery(bool remove_timestamp) { return data.getQuery(remove_timestamp); }

  void display()
  {
    std::cerr << "    " << getQuery(true) << std::endl;
  }
};


class QueryLogEntries : public QueryEntries {
public:
  typedef std::deque<QueryLogData> Entries;
  Entries entries;
  // keep track which query is the last one of each connection
  boost::unordered_set<const char*> last_query_of_conn;

  boost::shared_ptr<QueryEntry> popEntry() {
    boost::shared_ptr<QueryEntry> entry;
    if (!entries.empty()) {
      QueryLogData data = entries.front();
      entry = boost::make_shared<QueryLogEntry>(data);
      entries.pop_front();
      if (last_query_of_conn.count(data.data.data()))
        entry->set_shutdown();
    }
    return entry;
  }

  virtual void setShutdownOnLastQueryOfConn();
};

#ifdef __cplusplus
}
#endif

#endif /* PERCONA_PLAYBACK_QUERY_LOG_H */
