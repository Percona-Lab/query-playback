/* BEGIN LICENSE
 * Copyright (C) 2011 Percona Inc.
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

#ifndef PERCONA_PLAYBACK_PLUGIN_H
#define PERCONA_PLAYBACK_PLUGIN_H

#include <stdint.h>
#include <percona_playback/visibility.h>
#include <percona_playback/version.h>

namespace percona_playback {

class plugin
{
 public:
  typedef struct
  {
    uint32_t api_version;
    const char* name;
    const char* version;
    const char* author;
    const char* title;
    const char* license;
  } definition;
};

#define PERCONA_PLAYBACK_QUOTE(s) #s
#define PERCONA_PLAYBACK_QUOTE_VALUE(s) PERCONA_PLAYBACK_QUOTE(s)

#define PANDORA_CPP_NAME(x) _percona_playback_ ## x ## _plugin_
#define PANDORA_PLUGIN_NAME(x) PANDORA_CPP_NAME(x)

#define PERCONA_PLAYBACK_PLUGIN()			    \
  PERCONA_PLAYBACK_API percona_playback::plugin::definition	\
    PANDORA_PLUGIN_NAME(PANDORA_MODULE_NAME) =			\
    {							    \
      PERCONA_PLAYBACK_VERSION_ID,			    \
      PERCONA_PLAYBACK_QUOTE_VALUE(PANDORA_MODULE_NAME),    \
      PANDORA_MODULE_VERSION,				    \
      PANDORA_MODULE_AUTHOR,				    \
      PANDORA_MODULE_TITLE,				    \
      PERCONA_PLAYBACK_QUOTE_VALUE(PANDORA_MODULE_LICENSE)  \
    }

} /* namespace percona_playback */

#endif /* PERCONA_PLAYBACK_PLUGIN_H */
