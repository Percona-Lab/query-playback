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

#ifndef PERCONA_PLAYBACK_QUERY_ENTRY_H
#define PERCONA_PLAYBACK_QUERY_ENTRY_H

#include <boost/shared_ptr.hpp>
#include <stdint.h>

class DBThread;

class QueryEntry
{
protected:
  uint64_t thread_id;
  bool shutdown;
public:
  QueryEntry() : thread_id(0), shutdown(false) {};
  QueryEntry(uint64_t _thread_id, bool _shutdown) :
    thread_id(_thread_id), shutdown(_shutdown) {}
  virtual ~QueryEntry() {}

  void set_shutdown() { shutdown= true; }
  bool is_shutdown() const { return shutdown; }
  virtual bool is_quit()= 0;

  uint64_t getThreadId() const { return thread_id; }

  virtual void execute(DBThread *t)= 0;

};

class FinishEntry : public QueryEntry
{
public:
  FinishEntry(uint64_t _thread_id) :
    QueryEntry (_thread_id, true) {}
  bool is_quit() { return false; }

  void execute(DBThread *) {}
};

typedef boost::shared_ptr<QueryEntry> QueryEntryPtr;
#endif /* PERCONA_PLAYBACK_QUERY_ENTRY_H */
