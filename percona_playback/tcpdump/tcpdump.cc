/* BEGIN LICENSE
 * Copyright (C) 2011-2012 Percona Inc.
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

#include "tcpdump.h"
#include <stdio.h>

boost::program_options::options_description*
TcpDumpParserPlugin::getProgramOptions()
{
  options.add_options()
    ("tcpdump-file",
     boost::program_options::value<std::string>(&file_name),
     "Tcpdump file name");

  return &options;
}

int
TcpDumpParserPlugin::processOptions(
  boost::program_options::variables_map &vm)
{
  if (!active &&
      (vm.count("tcpdump-file")))
  {
      fprintf(stderr, 
              gettext("tcpdump plugin is not selected, "
                      "you shouldn't use this plugin-related "
                      "command line options\n"));
      return -1;
  }
  return 0;
}

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("tcpdump", new TcpDumpParserPlugin("tcpdump"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
