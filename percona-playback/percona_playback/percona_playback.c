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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "percona_playback.h"

struct percona_playback_st
{
  const char *name;
};

percona_playback_st *percona_playback_create(const char *name)
{
  percona_playback_st *the_percona_playback=
    (percona_playback_st *)malloc(sizeof(percona_playback_st));
  assert(the_percona_playback);
  the_percona_playback->name= name;
  return the_percona_playback;
}

void percona_playback_destroy(percona_playback_st **the_percona_playback)
{
  if (the_percona_playback)
  {
    free(*the_percona_playback);
    *the_percona_playback= NULL;
  }
}

const char *percona_playback_get_name(const percona_playback_st *the_percona_playback)
{
  if (the_percona_playback)
    return the_percona_playback->name;
  return NULL;
}

