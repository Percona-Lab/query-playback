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

#include "config.h"

#include <percona_playback/db_thread.h>
#include "percona_playback/query_result.h"

#include <assert.h>

#include <boost/bind.hpp>

extern std::string g_session_init_query;

void DBThread::init_session()
{
  if (g_session_init_query.empty())
    return;
  QueryResult r;
  QueryResult er;
  execute_query(g_session_init_query, &r, er);
}

void DBThread::run()
{
  connect_and_init_session();

  while (true)
  {
    QueryEntryPtr query;
    queries->pop(query);

    if (query->is_shutdown())
      break;

    if (query->is_quit())
    {
      disconnect();
      connect_and_init_session();
      continue;
    }

    query->execute(this);
  }

  disconnect();
  return;
}

void 
DBThread::start_thread()
{
  assert(!thread.joinable());
  thread= boost::thread(boost::bind(&DBThread::run, this));
}
