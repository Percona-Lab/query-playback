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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <percona_playback/percona_playback.h>

/**
 * @TODO Actually write a real test suite here
 */
int main(int argc, char **argv)
{
  (void)argc; (void)argv;

  fprintf(stderr, "Working dir: %s\n\n", get_current_dir_name());

  percona_playback_st *the_percona_playback= percona_playback_create("test_Percona Playback");
  assert(the_percona_playback);

  char dbplugin_opt[] = "--db-plugin=null";
  char stdin_opt[] = "--query-log-stdin";

  char *dbplugin_argv[3];
  dbplugin_argv[0]= argv[0];
  dbplugin_argv[1]= dbplugin_opt;
  dbplugin_argv[2]= stdin_opt;

  const char *query_log = SRCDIR"/percona_playback/test/basic-slow.log";
  int fd = open(query_log, O_RDONLY);
  if (fd < 0)
  {
    fprintf(stderr, "Error: open file '%s' error: %s\n", query_log, strerror(errno));
    return -1;
  }

  if (dup2(fd, STDIN_FILENO) < 0)
  {
    fprintf(stderr, "Error: can't dup2 to stdin: %s\n", strerror(errno));
    return -1;
  }

  assert(0 == percona_playback_argv(the_percona_playback, 3, dbplugin_argv));

  struct percona_playback_run_result *r= percona_playback_run(the_percona_playback);

  assert(r->err == 0);
  assert(r->n_queries == 17);
  assert(r->n_log_entries = 17);

  percona_playback_destroy(&the_percona_playback);
  return r->err;
}
