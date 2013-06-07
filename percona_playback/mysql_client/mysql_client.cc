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

bool MySQLDBThread::connect()
{
  mysql_init(&handle);
  if (!mysql_real_connect(&handle,
			  options->host.c_str(),
			  options->user.c_str(),
		          options->password.c_str(),
		          options->schema.c_str(),
		          options->port,
		          "",
		          CLIENT_MULTI_STATEMENTS))
  {
    fprintf(stderr, "Can't connect to server: %s\n",
	    mysql_error(&handle));
    return false;
  }
  return true;
}

void MySQLDBThread::disconnect()
{
  mysql_close(&handle);
}

void MySQLDBThread::execute_query(const std::string &query, QueryResult *r,
				  const QueryResult &)
{
  int mr;
  for(unsigned i = 0; i < max_err_num; ++i)
  {
    mr= mysql_real_query(&handle, query.c_str(), query.length());
    r->setError(mr);
    if(mr != 0)
    {
      fprintf(stderr,
	      "Error during query: %s, number of tries %u\n",
	      mysql_error(&handle),
	      i);
      disconnect();
      connect_and_init_session();
    }
    else
    {
      r->setWarningCount(mysql_warning_count(&handle));
      r->setRowsSent(0);
      do
      {
        MYSQL_RES* mysql_res= NULL;

        mysql_res= mysql_store_result(&handle);

        if (mysql_res != NULL)
	{
          r->setRowsSent(mysql_num_rows(mysql_res));
          mysql_free_result(mysql_res);
	}

      } while(!mysql_next_result(&handle));

      break;
    }
  }
}

void MySQLDBThread::run()
{
  mysql_thread_init();
  DBThread::run();
  mysql_thread_end();
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

    if (!active)
      return 0;

    if (!mysql_thread_safe())
    {
      fprintf(stderr, "libmysqlclient is not thread safe\n");
      return -1;
    }

    if (mysql_library_init(0, NULL, NULL))
    {
      fprintf(stderr, "could not initialize mysql library\n");
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
