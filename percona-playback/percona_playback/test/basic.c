/* BEGIN LICENSE
 * Copyright (C) 2011 Stewart Smith <stewart@flamingspork.com>
 * This program is free software: you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License version 3, as published 
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
  return 0;
}
