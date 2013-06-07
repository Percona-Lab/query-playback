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
#include <assert.h>
#include "percona_playback/percona_playback.h"
#include "percona_playback/plugin.h"

const size_t pp_argc= 4;

int main(int, char **argv)
{
  percona_playback_st *the_percona_playback= percona_playback_create("test_Percona Playback");
  assert(the_percona_playback);

  char *pp_argv[pp_argc];
  char pp_argv_0[]= "--db-plugin=null";
  char pp_argv_1[]= "--input-plugin=tcpdump";
  char pp_argv_2[]= "--tcpdump-file="
                      SRCDIR
                      "/percona_playback/test"
                      "/tcpdump_stress_test.dump";


  pp_argv[0]= argv[0];
  pp_argv[1]= pp_argv_0;
  pp_argv[2]= pp_argv_1;
  pp_argv[3]= pp_argv_2;

  assert(0 == percona_playback_argv(the_percona_playback,
                                    pp_argc,
                                    pp_argv));

  struct percona_playback_run_result *r=
    percona_playback_run(the_percona_playback);

  assert(r->err == 0);
  assert(r->n_queries == 9088);
  assert(r->n_log_entries == 62306);

  percona_playback_destroy(&the_percona_playback);
  return r->err;
}
