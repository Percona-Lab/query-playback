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

#ifndef PERCONA_PLAYBACK_QUERY_ENTRY_H
#define PERCONA_PLAYBACK_QUERY_ENTRY_H

#include <boost/shared_ptr.hpp>
#include <stdint.h>

class DBThread;

class QueryEntry
{
protected:
  bool shutdown;
public:
  QueryEntry(bool _shutdown = false) : shutdown(_shutdown) {}
  virtual ~QueryEntry() {}

  void set_shutdown() { shutdown= true; }
  bool is_shutdown() const { return shutdown; }
  virtual bool is_quit() const= 0;

  virtual uint64_t getThreadId() const  = 0;

  virtual void execute(DBThread *t)= 0;
};

class FinishEntry : public QueryEntry
{
public:
  FinishEntry() : QueryEntry (true) {}
  virtual bool is_quit() const { return false; }
  virtual uint64_t getThreadId() const { return 0; }
  virtual void execute(DBThread *) {}
};

class QueryEntries {
private:
  uint64_t num_entries, num_queries;

public:
  QueryEntries() : num_entries(0), num_queries(0) {}
  virtual ~QueryEntries() {}

  uint64_t getNumEntries() const { return num_entries; }
  uint64_t getNumQueries() const { return num_queries; }
  void setNumEntries(uint64_t num) { num_entries = num; }
  void setNumQueries(uint64_t num) { num_queries = num; }

  // this method returns the next query and a null ptr when no entry is left
  virtual boost::shared_ptr<QueryEntry> popEntry() = 0;

  // sets shutdown to true on the last query of every connection/thread id.
  virtual void setShutdownOnLastQueryOfConn() = 0;
};

typedef boost::shared_ptr<QueryEntry> QueryEntryPtr;
typedef boost::shared_ptr<QueryEntries> QueryEntriesPtr;
#endif /* PERCONA_PLAYBACK_QUERY_ENTRY_H */
