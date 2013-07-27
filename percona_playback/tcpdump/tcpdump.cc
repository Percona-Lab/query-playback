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

#include <config.h>
#include "tcpdump.h"
#include "pcap_packets_parser.h"

#include "percona_playback/percona_playback.h"
#include <percona_playback/gettext.h>

#include <stdio.h>
#include <pcap.h>

#define FILTER_EXP_SIZE     11     // max filter expression size

std::string         g_tcpdump_file_name;
uint16_t            g_tcpdump_port;
TcpdumpPluginMode   g_tcpdump_mode;

static std::istream& operator>>(std::istream& in, TcpdumpPluginMode& unit)
{
    std::string token;
    in >> token;
    if (token == "fast")
        unit = TCPDUMP_MODE_FAST;
    else if (token == "accurate")
        unit = TCPDUMP_MODE_ACCURATE;
    else
      throw
        boost::program_options::invalid_option_value(
            _("Allowed input-type values are (fast|accurate)")
        );
    return in;
}

boost::program_options::options_description*
TcpDumpParserPlugin::getProgramOptions()
{
  options.add_options()
    ("tcpdump-file",
     boost::program_options::value<std::string>(&g_tcpdump_file_name),
     _("Tcpdump file name (produced with \"tcpdump -w file\")"))
    ("tcpdump-port",
     boost::program_options::value<uint16_t>(&g_tcpdump_port)
       ->default_value(3306),
     _("Tcpdump port"))
    ("tcpdump-mode",
     boost::program_options::value<TcpdumpPluginMode>(&g_tcpdump_mode)
       ->multitoken()->default_value(TCPDUMP_MODE_FAST, "fast"),
     _("The mode of tcpdump plugin (fast|accurate), in 'fast' mode "
     "the plugin executes queries as fast as it can whereas in "
     "'accurate' mode the plugin preserves queries execution time "
     "and pauses between queries"))
    ;

  return &options;
}

int
TcpDumpParserPlugin::processOptions(
  boost::program_options::variables_map &vm)
{
  if (!active &&
      (vm.count("tcpdump-file") ||
       !vm["tcpdump-port"].defaulted() ||
       !vm["tcpdump-mode"].defaulted()))
  {
    fprintf(stderr, _("tcpdump plugin is not selected, "
                      "you shouldn't use this plugin-related "
                      "command line options\n"));
    return -1;
  }

  if (!active)
    return 0;

  if (!vm.count("tcpdump-file"))
  {
    fprintf(stderr,
            gettext("tcpdump-file argument is requred\n"));
    return -1;
  }

  return 0;
}

void
TcpDumpParserPlugin::run(percona_playback_run_result  &r)
{
  char                errbuf[PCAP_ERRBUF_SIZE];
  pcap_t              *handle;
  struct              bpf_program fp;
  char                filter_exp[11];
  PcapPacketsParser   packets_parser;


  if (g_tcpdump_file_name.empty())
  {
    r.err= -1;
    return;
  }

  handle = pcap_open_offline(g_tcpdump_file_name.c_str(), errbuf);

  if (handle == NULL)
  {
    fprintf(stderr, _("Couldn't open file %s: %s\n"),
	    g_tcpdump_file_name.c_str(), errbuf);
    r.err= -1;
    return;
  }

  snprintf(filter_exp, FILTER_EXP_SIZE, "port %u", (unsigned)g_tcpdump_port);

  if(pcap_compile(handle, &fp, filter_exp, 0, 0) == -1)
  {
    fprintf(stderr, _("Couldn't parse filter '%s': %s\n"),
	    filter_exp, pcap_geterr(handle));
    r.err= -1;
    return;
  }

  if (pcap_setfilter(handle, &fp) == -1)
  {
    fprintf(stderr, _("Couldn't install filter '%s': %s\n"),
	    filter_exp, pcap_geterr(handle));
    r.err= -1;
    return;
  }

  if (pcap_loop(handle,
                -1,
                PcapPacketsParser::Process,
                (u_char *)&packets_parser) < 0)
  {
    std::cerr << "pcap_loop error "
              << pcap_geterr(handle) << std::endl;
    r.err= -1;
    return;
  }

  /* Wait for unfinished DBThreads */
  packets_parser.WaitForUnfinishedTasks();

  const TcpdumpMysqlParserStats &stats= packets_parser.GetStats();
 
  r.n_log_entries=   stats.nr_of_parsed_packets;
  r.n_queries=       stats.nr_of_parsed_queries;
  r.err=             stats.nr_of_parsing_errors;
  
  return;
}

static void init_plugin(percona_playback::PluginRegistry &r)
{
  r.add("tcpdump", new TcpDumpParserPlugin("tcpdump"));
}

PERCONA_PLAYBACK_PLUGIN(init_plugin);
