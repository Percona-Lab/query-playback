/* BEGIN LICENSE
 * Copyright (C) 2011 Percona Inc.
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

#include <percona_playback/plugin.h>
#include <percona_playback/mysql_client/mysql_client.h>
#include <percona_playback/query_result.h>

void MySQLDBThread::connect()
{
  mysql_init(&handle);
  mysql_real_connect(&handle,
		     "127.0.0.1",
		     "root",
		     "",
		     "test",
		     13010,
		     "",
		     0);
}

void MySQLDBThread::disconnect()
{
  mysql_close(&handle);
}

void MySQLDBThread::execute_query(const std::string &query, QueryResult *r)
{
  int mr= mysql_real_query(&handle, query.c_str(), query.length());
  if(mr != 0)
  {
    r->setError(mr);
  }
  else
  {
    MYSQL_RES* mysql_res= NULL;

    //    r->setAffectedRows(mysql_affected_rows(&handle));
    r->setError(mr);

    mysql_res= mysql_store_result(&handle);
    mysql_free_result(mysql_res);
  }
}

class MySQLDBClientPlugin : public percona_playback::DBClientPlugin
{
public:
  MySQLDBClientPlugin(std::string _name) : DBClientPlugin(_name) {};

  virtual DBThread* create(uint64_t _thread_id) {
    return new MySQLDBThread(_thread_id);
  }
};

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("libmysqlclient", new MySQLDBClientPlugin("libmysqlclient"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
