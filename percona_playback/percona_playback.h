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

/**
 * @file
 * @brief A Basic Example Class
 */

#ifndef PERCONA_PLAYBACK_PERCONA_PLAYBACK_H
#define PERCONA_PLAYBACK_PERCONA_PLAYBACK_H

#include "percona_playback/visibility.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Forward declaration for main struct implementing percona_playback
 */
typedef struct percona_playback_st percona_playback_st;

PERCONA_PLAYBACK_API
percona_playback_st *percona_playback_create(const char *name);

PERCONA_PLAYBACK_API
void percona_playback_destroy(percona_playback_st **the_percona_playback);

PERCONA_PLAYBACK_API
const char *percona_playback_get_name(const percona_playback_st *the_percona_playback);

PERCONA_PLAYBACK_API
int percona_playback_argv(percona_playback_st *the_percona_playback,
			  int argc, char** argv);

struct percona_playback_run_result {
  int err;
  uint64_t n_log_entries;
  uint64_t n_queries;
};

PERCONA_PLAYBACK_API
struct percona_playback_run_result *percona_playback_run(const percona_playback_st *the_percona_playback);

PERCONA_PLAYBACK_API
int percona_playback_run_all(const percona_playback_st *the_percona_playback);

#ifdef __cplusplus
}
#endif

#endif /* PERCONA_PLAYBACK_PERCONA_PLAYBACK_H */
