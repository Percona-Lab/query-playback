/* BEGIN LICENSE
 * Copyright (C) 2011-2013 Percona Ireland Ltd
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

#include <stdio.h>

#include "percona_playback/percona_playback.h"

int main(int argc, char** argv)
{
  int exit_code= 0;

  percona_playback_st *the_percona_playback= percona_playback_create("Percona Playback");

  int r= percona_playback_argv(the_percona_playback, argc, argv);
  if (r > 0)
  {
    goto exit;
  }
  if (r < 0)
  {
    exit_code= r;
    goto exit;
  }

  r= percona_playback_run_all(the_percona_playback);
  exit_code= r;

 exit:
  percona_playback_destroy(&the_percona_playback);
  return exit_code;
}
