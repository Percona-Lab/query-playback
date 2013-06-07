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
#include <string>
#include <cstring>
#include <assert.h>

#include <percona_playback/percona_playback.h>

/**
 * @TODO Actually write a real test suite here
 */
int main(int argc, char **argv)
{
  (void)argc; (void)argv;
  percona_playback_st *the_percona_playback= percona_playback_create("test_Percona Playback");
  assert(the_percona_playback);

  std::string help_str("--help");
  char *help= new char[help_str.length()+1];
  strcpy(help, help_str.c_str());
  char **help_argv= new char*[2];
  help_argv[0]= argv[0];
  help_argv[1]= help;

  assert(1 == percona_playback_argv(the_percona_playback, 2, help_argv));
  delete [] help;
  delete [] help_argv;

  std::string version_str("--version");
  char *version= new char[version_str.length()+1];
  strcpy(version, version_str.c_str());
  char **version_argv= new char*[2];
  version_argv[0]= argv[0];
  version_argv[1]= version;

  assert(2 == percona_playback_argv(the_percona_playback, 2, version_argv));
  delete [] version;
  delete [] version_argv;


  percona_playback_destroy(&the_percona_playback);
  return 0;
}
