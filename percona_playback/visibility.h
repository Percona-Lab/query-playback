/*
 * Percona Playback
 *
 * Copyright (C) 2011-2013 Percona Ireland Ltd.
 * Copyright (C) 2010 Eric Day (eday@oddments.org)
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license. See the
 * COPYING file in the root project directory for full text.
 */

/**
 * @file
 * @brief Common Macro Definitions
 */

#ifndef PERCONA_PLAYBACK_VISIBILITY_H
#define PERCONA_PLAYBACK_VISIBILITY_H

/**
 * Be sure to put PERCONA_PLAYBACK_API in front of all public API symbols, or one of
 * the other macros as appropriate. The default visibility without a macro is
 * to be hidden (PERCONA_PLAYBACK_LOCAL).
 */

#if defined(BUILDING_PERCONA_PLAYBACK) && defined(HAVE_VISIBILITY)
# if defined(__GNUC__)
#  define PERCONA_PLAYBACK_API __attribute__ ((visibility("default")))
#  define PERCONA_PLAYBACK_INTERNAL_API __attribute__ ((visibility("hidden")))
#  define PERCONA_PLAYBACK_API_DEPRECATED __attribute__ ((deprecated,visibility("default")))
#  define PERCONA_PLAYBACK_LOCAL  __attribute__ ((visibility("hidden")))
# elif (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)) || (defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x550))
#  define PERCONA_PLAYBACK_API __global
#  define PERCONA_PLAYBACK_INTERNAL_API __hidden
#  define PERCONA_PLAYBACK_API_DEPRECATED __global
#  define PERCONA_PLAYBACK_LOCAL __hidden
# elif defined(_MSC_VER)
#  define PERCONA_PLAYBACK_API extern __declspec(dllexport)
#  define PERCONA_PLAYBACK_INTERNAL_API extern __declspec(dllexport)
#  define PERCONA_PLAYBACK_DEPRECATED_API extern __declspec(dllexport)
#  define PERCONA_PLAYBACK_LOCAL
# endif
#else  /* defined(BUILDING_PERCONA_PLAYBACK) && defined(HAVE_VISIBILITY) */
# if defined(_MSC_VER)
#  define SCALESTACK_API extern __declspec(dllimport)
#  define PERCONA_PLAYBACK_INTERNAL_API extern __declspec(dllimport)
#  define PERCONA_PLAYBACK_API_DEPRECATED extern __declspec(dllimport)
#  define PERCONA_PLAYBACK_LOCAL
# else
#  define PERCONA_PLAYBACK_API
#  define PERCONA_PLAYBACK_INTERNAL_API
#  define PERCONA_PLAYBACK_API_DEPRECATED
#  define PERCONA_PLAYBACK_LOCAL
# endif /* defined(_MSC_VER) */
#endif  /* defined(BUILDING_PERCONA_PLAYBACK) && defined(HAVE_VISIBILITY) */


#endif /* PERCONA_PLAYBACK_VISIBILITY_H */
