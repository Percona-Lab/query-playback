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

#include <config.h>

#include <vector>
#include <string>
#include <percona_playback/plugin.h>
#include <percona_playback/plugin_load_list.h>
#include <percona_playback/tokenize.h>

namespace percona_playback {

std::vector<std::string> loaded_plugin_names;

void load_plugins()
{
  std::vector<std::string> builtin_load_list;
  tokenize(PANDORA_BUILTIN_LOAD_LIST, builtin_load_list, ",", true);

  loaded_plugin_names= builtin_load_list;
}

} /* namespace percona_playback */
