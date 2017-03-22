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

#include <percona_playback/plugin.h>
#include <percona_playback/mysql_client/mysql_client.h>
#include <percona_playback/query_result.h>

#include <boost/make_shared.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>

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
    socket("/var/lib/mysql/mysql.sock"),
    port(3306),
    max_retries(9)
  {
  }

  std::string host;
  std::string user;
  std::string password;
  std::string schema;
  std::string socket;
  unsigned int port;
  unsigned int max_retries;
  boost::regex filter_error_regex;
};

bool MySQLDBThread::should_print_error(const char* error) {
  return options->filter_error_regex.empty() || !boost::regex_search(error, options->filter_error_regex);
}

bool MySQLDBThread::connect()
{
  mysql_init(&handle);
  if (!mysql_real_connect(&handle,
			  options->host.c_str(),
			  options->user.c_str(),
		          options->password.c_str(),
		          options->schema.c_str(),
		          options->port,
		          options->socket.c_str(),
		          CLIENT_MULTI_STATEMENTS))
  {
    ++num_connect_errors;
    if (should_print_error(mysql_error(&handle)))
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
  for(unsigned i = 0; i < options->max_retries + 1; ++i)
  {
    mr= mysql_real_query(&handle, query.c_str(), query.length());
    r->setError(mr);
    if(mr != 0)
    {
      if (should_print_error(mysql_error(&handle)))
        fprintf(stderr,
                "Error during query: %s, number of tries %u of %u\n",
                mysql_error(&handle),
                i + 1,
                options->max_retries + 1);
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

bool MySQLDBThread::test_connect(MySQLOptions* opt) {
  boost::shared_ptr<MySQLDBThread> thread = boost::make_shared<MySQLDBThread>(0, opt);
  thread->queries->set_capacity(1);
  thread->queries->push(boost::make_shared<FinishEntry>());
  thread->start_thread();
  thread->join();
  return thread->num_connect_errors == 0;
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
    ("mysql-socket", po::value<std::string>(), _("MySQL Socket to connect to when mysql-host=localhost"))
    ("mysql-port", po::value<unsigned int>(), _("MySQL port number"))
    ("mysql-max-retries", po::value<unsigned int>(), _("How often should we retry a query which returned an error"))
    ("mysql-filter-error", po::value<std::string>(), _("Don't print error messages which contain the specified regex"
                                                       "(use \".*\" to suppress all errors)"))
    ("mysql-test-connect", po::value<bool>(), _("Per default we do a test connection to the MySQL server to check"
                                                " if the connection settings are correct and exit if it fails"))
    ;

    return &mysql_options;
  }

  virtual int processOptions(boost::program_options::variables_map &vm) {
    if (!active &&
        (vm.count("mysql-host") ||
         vm.count("mysql-username") ||
         vm.count("mysql-password") ||
         vm.count("mysql-schema") ||
         vm.count("mysql-socket") ||
         vm.count("mysql-port")))
    {
      fprintf(stderr,
              "libmysqlclient plugin is not selected, you shouldn't use this plugin-related command line options\n");

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

    if (vm.count("mysql-socket"))
    {
      options.socket= vm["mysql-socket"].as<std::string>();
    }

    if (vm.count("mysql-port"))
    {
      options.port= vm["mysql-port"].as<unsigned int>();
    }

    if (vm.count("mysql-max-retries"))
    {
      options.max_retries= vm["mysql-max-retries"].as<unsigned int>();
    }

    if (vm.count("mysql-filter-error") && !vm["mysql-filter-error"].as<std::string>().empty())
    {
      options.filter_error_regex = boost::regex(vm["mysql-filter-error"].as<std::string>());
    }

    bool test_connect = true;
    if (vm.count("mysql-test-connect"))
    {
      test_connect= vm["mysql-test-connect"].as<bool>();
    }
    if (test_connect && !MySQLDBThread::test_connect(&options))
    {
      fprintf(stderr, "Exiting because '--mysql-test-connect' is enabled\n"
                      "\tuse '--mysql-test-connect=off' to disable the check\n");
      fflush(stderr);
      exit(EXIT_FAILURE);
    }

    return 0;
  }

};

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("libmysqlclient", new MySQLDBClientPlugin("libmysqlclient"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
