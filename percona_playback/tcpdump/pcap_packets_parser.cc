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
#include "pcap_packets_parser.h"
#include "sniff_headers.h"
#include "tcpdump.h"

#include "percona_playback/db_thread.h"
#include "percona_playback/plugin.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <boost/foreach.hpp>
#include <assert.h>

#define SNAP_LEN            16500  // pcap's max capture size

extern percona_playback::DBClientPlugin *g_dbclient_plugin;
extern percona_playback::DispatcherPlugin *g_dispatcher_plugin;

void
PcapPacketsParser::ParsePkt(const struct pcap_pkthdr *header,
                            const u_char *packet)
{
  ConnectionState::Origin     origin= ConnectionState::UNDEF;
  const struct                sniff_ip *ip;
  const struct                sniff_tcp *tcp;
  const uint8_t               *mysql;
  size_t                      size_ip;
  size_t                      size_tcp;
  size_t                      size_mysql;

  assert(header);
  assert(packet);

  if(header->len > SNAP_LEN) {
    std::cerr << "Captured packet too large: "
              << header->len 
              << " bytes, Max capture packet size: "
              << SNAP_LEN << " bytes" << std::endl;
     return;
  }

  /* ethernet=     (struct sniff_ethernet*)(packet); */
  ip=           (struct sniff_ip*)(packet + SIZE_ETHERNET);
  size_ip=      IP_HL(ip) * 4;
  tcp=          (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);
  size_tcp=     TH_OFF(tcp) * 4;
  mysql=        (packet + SIZE_ETHERNET + size_ip + size_tcp);
  size_mysql=   ntohs(ip->ip_len) - size_ip - size_tcp;


  AddrPort addr_port;

  if (ntohs(tcp->th_sport) == g_tcpdump_port)
  {
    addr_port.address= ip->ip_dst.s_addr;
    addr_port.port= tcp->th_dport;
    origin= ConnectionState::SERVER;
  }
  else
  {
    addr_port.address= ip->ip_src.s_addr;
    addr_port.port= tcp->th_sport;
    origin= ConnectionState::CLIENT;
  }

  /* The connection was closed. */
  if (size_mysql == 0 &&
      ((tcp->th_flags & TH_FIN) || (tcp->th_flags & TH_RST)))
  {
    g_dispatcher_plugin->finish_and_wait(addr_port.ThreadId());
    RemoveConnectionState(addr_port.ThreadId());
    return;
  }

  /* The mysql packet is too small. It may be due to tcp service packets. */
  if (size_mysql < MysqlPkt::header_size())
    return;

  if (!was_first_packet)
  {
    gettimeofday(&first_packet_timestamp, 0);
    first_packet_pcap_timestamp= header->ts;
    was_first_packet= true;
  }

  /* If there is no DBThread with such id create it */
  boost::shared_ptr<ConnectionState>
    state= GetConnectionState(addr_port.ThreadId());

  assert(state.get());

  state->SetCurrentOrigin(origin);
  state->ProcessMysqlPkts(state,
			  mysql,
                          size_mysql,
                          header->ts,
                          addr_port,
                          stats);
}

void
PcapPacketsParser::WaitForUnfinishedTasks()
{
  g_dispatcher_plugin->finish_all_and_wait();
}

boost::shared_ptr<ConnectionState>
PcapPacketsParser::GetConnectionState(uint64_t thread_id)
{
  Connections::iterator it = connections.find(thread_id);
  if (it == connections.end())
    return CreateConnectionState(thread_id);
  return it->second;
}

boost::shared_ptr<ConnectionState>
PcapPacketsParser::CreateConnectionState(uint64_t thread_id)
{
  boost::shared_ptr<ConnectionState> state(new ConnectionState(thread_id));
  state->last_executed_query_info.end_pcap_timestamp=
    first_packet_pcap_timestamp;
  state->last_executed_query_info.end_timestamp=
    first_packet_timestamp;
  connections[thread_id] = state;
  return state;
}

void
PcapPacketsParser::RemoveConnectionState(uint64_t thread_id)
{
  connections.erase(thread_id);
}

