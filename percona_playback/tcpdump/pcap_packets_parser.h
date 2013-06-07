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

#ifndef PERCONA_PLAYBACK_PCAP_PACKETS_PARSER_H
#define PERCONA_PLAYBACK_PCAP_PACKETS_PARSER_H

#include "connection_state.h"
#include "tcpdump_mysql_parser_stats.h"

#include <pcap.h>
#include <tbb/concurrent_queue.h>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

class DBThread;

class PcapPacketsParser
{
  TcpdumpMysqlParserStats    stats;
  timeval                    first_packet_timestamp;
  timeval                    first_packet_pcap_timestamp;
  bool                       was_first_packet;
  typedef boost::unordered_map<uint64_t,
			       boost::shared_ptr<ConnectionState> >
          Connections;
  Connections		     connections;

public:

  PcapPacketsParser() : was_first_packet(false), connections(10000)
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


private:
  boost::shared_ptr<ConnectionState> CreateConnectionState(uint64_t thread_id);
  boost::shared_ptr<ConnectionState> GetConnectionState(uint64_t thread_id);
  void RemoveConnectionState(uint64_t thread_id);

  void ParsePkt(const struct pcap_pkthdr *header,
                const u_char *packet);

};

#endif //PERCONA_PLAYBACK_PCAP_PACKETS_PARSER_H
