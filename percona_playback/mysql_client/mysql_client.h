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

#include <percona_playback/db_thread.h>
#include <mysql.h>

class QueryResult;
class MySQLOptions;

class MySQLDBThread : public DBThread
{
 private:
  MYSQL handle;
  MySQLOptions *options;
  static const unsigned max_err_num = 10;

 public:
  MySQLDBThread(uint64_t _thread_id, MySQLOptions *opt) :
    DBThread(_thread_id,
	     boost::shared_ptr<Queries>(new Queries())),
    options(opt)
  {
  }

  bool connect();
  void disconnect();
  void execute_query(const std::string &query, QueryResult *r,
		     const QueryResult &expected_result);
  void run();
};
