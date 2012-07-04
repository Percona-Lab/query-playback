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

#include <percona_playback/plugin.h>
#include <percona_playback/mysql_client/mysql_client.h>
#include <percona_playback/query_result.h>
#include <percona_playback/gettext.h>

#include <boost/program_options.hpp>

#include <stdio.h>
namespace po= boost::program_options;

class MySQLOptions
{
public:
  MySQLOptions() :
    host("127.0.0.1"),
    user("root"),
    password(""),
    schema("test"),
    port(3306)
  {
  }

  std::string host;
  std::string user;
  std::string password;
  std::string schema;
  unsigned int port;
};

void MySQLDBThread::connect()
{
  mysql_init(&handle);
  mysql_real_connect(&handle,
		     options->host.c_str(),
		     options->user.c_str(),
		     options->password.c_str(),
		     options->schema.c_str(),
		     options->port,
		     "",
		     0);
}

void MySQLDBThread::disconnect()
{
  mysql_close(&handle);
}

void MySQLDBThread::execute_query(const std::string &query, QueryResult *r,
				  const QueryResult &)
{
  int mr= mysql_real_query(&handle, query.c_str(), query.length());
  if(mr != 0)
  {
    r->setError(mr);
  }
  else
  {
    MYSQL_RES* mysql_res= NULL;

    r->setError(mr);
    r->setWarningCount(mysql_warning_count(&handle));

    mysql_res= mysql_store_result(&handle);

    if (mysql_res != NULL)
      r->setRowsSent(mysql_num_rows(mysql_res));
    else
      r->setRowsSent(0);

    mysql_free_result(mysql_res);
  }
}

class MySQLDBClientPlugin : public percona_playback::DBClientPlugin
{
private:
  MySQLOptions options;

public:
  MySQLDBClientPlugin(std::string _name) : DBClientPlugin(_name) {};

  virtual DBThread* create(uint64_t _thread_id) {
    return new MySQLDBThread(_thread_id, &options);
  }

  virtual boost::program_options::options_description* getProgramOptions() {
    static po::options_description mysql_options(_("MySQL Client Options"));
    mysql_options.add_options()
    ("mysql-host", po::value<std::string>(), _("Hostname of MySQL server"))
    ("mysql-username", po::value<std::string>(), _("Username to connect to MySQL"))
    ("mysql-password", po::value<std::string>(), _("Password for MySQL user"))
    ("mysql-schema", po::value<std::string>(), _("MySQL Schema to connect to"))
    ("mysql-port", po::value<unsigned int>(), _("MySQL port number"))
    ;

    return &mysql_options;
  }

  virtual int processOptions(boost::program_options::variables_map &vm) {
    if (!active &&
        (vm.count("mysql-host") ||
         vm.count("mysql-username") ||
         vm.count("mysql-password") ||
         vm.count("mysql-schema") ||
         vm.count("mysql-port")))
    {
      fprintf(stderr,
              gettext("libmysqlclient plugin is not selected, "
                      "you shouldn't use this plugin-related "
                      "command line options\n"));
      return -1;
    }

    if (vm.count("mysql-host"))
    {
      options.host= vm["mysql-host"].as<std::string>();
    }

    if (vm.count("mysql-username"))
    {
      options.user= vm["mysql-username"].as<std::string>();
    }

    if (vm.count("mysql-password"))
    {
      options.password= vm["mysql-password"].as<std::string>();
    }

    if (vm.count("mysql-schema"))
    {
      options.schema= vm["mysql-schema"].as<std::string>();
    }

    if (vm.count("mysql-port"))
    {
      options.port= vm["mysql-port"].as<unsigned int>();
    }

    return 0;
  }

};

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("libmysqlclient", new MySQLDBClientPlugin("libmysqlclient"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
