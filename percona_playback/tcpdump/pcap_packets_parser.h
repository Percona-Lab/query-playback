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

#ifndef PERCONA_PLAYBACK_PCAP_PACKETS_PARSER_H
#define PERCONA_PLAYBACK_PCAP_PACKETS_PARSER_H

#include "connection_state.h"
#include "tcpdump_mysql_parser_stats.h"

#include <pcap.h>
#include <tbb/concurrent_queue.h>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

class DBThread;

class PcapPacketsParser
{
  TcpdumpMysqlParserStats    stats;
  timeval                    first_packet_timestamp;
  timeval                    first_packet_pcap_timestamp;
  bool                       was_first_packet;

public:

  PcapPacketsParser() : was_first_packet(false)
  {
    first_packet_timestamp.tv_sec= first_packet_timestamp.tv_usec= 0;
    first_packet_pcap_timestamp.tv_sec= first_packet_pcap_timestamp.tv_usec= 0;
  }

  static void Process(u_char *arg,
                      const struct pcap_pkthdr *header,
                      const u_char *packet)
  {
    ((PcapPacketsParser *)arg)->ParsePkt(header, packet);
  }

  const TcpdumpMysqlParserStats &GetStats() const { return stats; }

  void WaitForUnfinishedTasks();

  void CreateConnectionState(DBThread *db_thread);

private:

  void ParsePkt(const struct pcap_pkthdr *header,
                const u_char *packet);

};

#endif //PERCONA_PLAYBACK_PCAP_PACKETS_PARSER_H
