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

#include <percona_playback/db_thread.h>

DBExecutorsTable db_executors;

bool DBThread::run()
{
  connect();

  QueryEntryPtr query;
  while (true)
  {
    queries.pop(query);
    if (query->is_shutdown())
    {
      disconnect();
      return true;
    }
    if (query->is_quit())
    {
      disconnect();
      connect();
      continue;
    }
    query->execute(this);
  }

  disconnect();
  return false;
}

void RunDBThread(DBThread* dbt, uint64_t thread_id)
{
  if (dbt->run() == false)
  {
    db_executors.erase(thread_id);
    delete dbt;
  }
}
