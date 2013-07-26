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

#include <config.h>
#include <percona_playback/plugin.h>
#include <percona_playback/query_result.h>
#include <percona_playback/gettext.h>

#include <boost/program_options.hpp>

#include <stdio.h>
namespace po= boost::program_options;


#include <percona_playback/db_thread.h>
#ifdef HAVE_LIBDRIZZLE_1_0_DRIZZLE_CLIENT_H
#include <libdrizzle-1.0/drizzle.h>
#include <libdrizzle-1.0/drizzle_client.h>
#else
#include <libdrizzle/drizzle.h>
#include <libdrizzle/drizzle_client.h>
#endif

class QueryResult;
class LibDrizzleOptions;

class LibDrizzleDBThread : public DBThread
{
 private:
  drizzle_st *driz;
  drizzle_con_st *client;
  LibDrizzleOptions *options;

 public:
  LibDrizzleDBThread(uint64_t _thread_id, LibDrizzleOptions *opt) :
    DBThread(_thread_id, boost::shared_ptr<Queries>(new Queries())),
    driz(NULL),
    options(opt)
  {
  }

  bool connect();
  void disconnect();
  void execute_query(const std::string &query, QueryResult *r,
		     const QueryResult &expected_result);
};

class LibDrizzleOptions
{
public:
  LibDrizzleOptions() :
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

bool LibDrizzleDBThread::connect()
{
  driz= drizzle_create(NULL);
  assert(driz != NULL);

  client= drizzle_con_add_tcp(driz, NULL,
			      options->host.c_str(),
			      options->port,
			      options->user.c_str(),
			      options->password.c_str(),
			      options->schema.c_str(), DRIZZLE_CON_MYSQL);
  return true;
}

void LibDrizzleDBThread::disconnect()
{
  drizzle_free(driz);
}

void LibDrizzleDBThread::execute_query(const std::string &query, QueryResult *r,
				       const QueryResult &)
{
  drizzle_result_st result;
  drizzle_return_t ret;
  drizzle_query(client, &result, query.c_str(), query.length(), &ret);

  if(ret != 0)
  {
    r->setError(ret);
  }
  else
  {
    drizzle_result_buffer(&result);

    r->setError(ret);
    r->setWarningCount(drizzle_result_warning_count(&result));

    r->setRowsSent(drizzle_result_row_count(&result));
  }

  drizzle_result_free_all(client);
}

class LibDrizzleDBClientPlugin : public percona_playback::DBClientPlugin
{
private:
  LibDrizzleOptions options;

public:
  LibDrizzleDBClientPlugin(std::string _name) : DBClientPlugin(_name) {};

  virtual DBThread* create(uint64_t _thread_id) {
    return new LibDrizzleDBThread(_thread_id, &options);
  }

  virtual boost::program_options::options_description* getProgramOptions() {
    static po::options_description libdrizzle_options(_("libdrizzle Client Options"));
    libdrizzle_options.add_options()
    ("libdrizzle-host", po::value<std::string>(), _("Hostname of MySQL/Drizzle server"))
    ("libdrizzle-username", po::value<std::string>(), _("Username to connect to MySQL/Drizzle"))
    ("libdrizzle-password", po::value<std::string>(), _("Password for MySQL/Drizzle user"))
    ("libdrizzle-schema", po::value<std::string>(), _("Schema to connect to"))
    ("libdrizzle-port", po::value<unsigned int>(), _("port number"))
    ;

    return &libdrizzle_options;
  }

  virtual int processOptions(boost::program_options::variables_map &vm) {
    if (!active &&
        (vm.count("libdrizzle-host") ||
         vm.count("libdrizzle-username") ||
         vm.count("libdrizzle-password") ||
         vm.count("libdrizzle-schema") ||
         vm.count("libdrizzle-port")))
    {
      fprintf(stderr,
              gettext("libdrizzle_client plugin is not selected, "
                      "you shouldn't use this plugin-related "
                      "command line options\n"));
      return -1;
    }

    if (vm.count("libdrizzle-host"))
    {
      options.host= vm["libdrizzle-host"].as<std::string>();
    }

    if (vm.count("libdrizzle-username"))
    {
      options.user= vm["libdrizzle-username"].as<std::string>();
    }

    if (vm.count("libdrizzle-password"))
    {
      options.password= vm["libdrizzle-password"].as<std::string>();
    }

    if (vm.count("libdrizzle-schema"))
    {
      options.schema= vm["libdrizzle-schema"].as<std::string>();
    }

    if (vm.count("libdrizzle-port"))
    {
      options.port= vm["libdrizzle-port"].as<unsigned int>();
    }

    return 0;
  }

};

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("libdrizzle", new LibDrizzleDBClientPlugin("libdrizzle"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
