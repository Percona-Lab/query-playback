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

#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <assert.h>
#include <unistd.h>

#include <percona_playback/percona_playback.h>

#include <boost/date_time/posix_time/posix_time.hpp>

/**
 * @TODO Actually write a real test suite here
 */
int main(int argc, char **argv)
{
  (void)argc; (void)argv;

  fprintf(stderr, "Working dir: %s\n\n", get_current_dir_name());

  percona_playback_st *the_percona_playback= percona_playback_create("test_Percona Playback");
  assert(the_percona_playback);

  std::string null_dbplugin("--db-plugin=null");
  char *dbplugin= new char[null_dbplugin.length()+1];
  strcpy(dbplugin, null_dbplugin.c_str());

  std::string querylog_str("--query-log-file=");
  querylog_str.append(SRCDIR);
  querylog_str.append("/percona_playback/test/preserve_query_time.log");

  char *querylog= new char[querylog_str.length()+1];
  strcpy(querylog, querylog_str.c_str());

  std::string preserve_time_str("--query-log-preserve-query-time");
  char *preserve_time= new char[preserve_time_str.length()+1];
  strcpy(preserve_time, preserve_time_str.c_str());

  char **dbplugin_argv= new char*[4];
  dbplugin_argv[0]= argv[0];
  dbplugin_argv[1]= dbplugin;
  dbplugin_argv[2]= querylog;
  dbplugin_argv[3]= preserve_time;

  assert(0 == percona_playback_argv(the_percona_playback, 4, dbplugin_argv));
  delete [] dbplugin;
  delete [] querylog;
  delete [] preserve_time;
  delete [] dbplugin_argv;

  boost::posix_time::ptime start_time;
  start_time= boost::posix_time::microsec_clock::universal_time();

  struct percona_playback_run_result *r= percona_playback_run(the_percona_playback);

  boost::posix_time::ptime end_time;
  end_time= boost::posix_time::microsec_clock::universal_time();

  boost::posix_time::time_period duration(start_time, end_time);

  assert(duration.length().total_microseconds() > 250000);

  assert(r->err == 0);
  assert(r->n_queries == 2);
  assert(r->n_log_entries = 2);

  percona_playback_destroy(&the_percona_playback);
  return r->err;
}
