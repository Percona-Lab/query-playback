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

#ifndef PERCONA_PLAYBACK_TCPDUMP_H
#define PERCONA_PLAYBACK_TCPDUMP_H

#include <percona_playback/plugin.h>

#include <boost/program_options.hpp>

class TcpDumpParserPlugin : public percona_playback::InputPlugin
{

public:
  TcpDumpParserPlugin(const std::string &name) :
    InputPlugin(name),
    options("Tcpdump Options")
  {};

  virtual boost::program_options::options_description* getProgramOptions();

  virtual int processOptions(boost::program_options::variables_map &vm);

  virtual void run(percona_playback_run_result  &result) {}


  std::string file_name;
  boost::program_options::options_description options;
};

#endif /* PERCONA_PLAYBACK_TCPDUMP_PARSER_H */
