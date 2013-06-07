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

#ifndef PERCONA_PLAYBACK_TCPDUMP_QUERY_ENTRIES_H
#define PERCONA_PLAYBACK_TCPDUMP_QUERY_ENTRIES_H

#include "percona_playback/query_entry.h"
#include "percona_playback/query_result.h"
#include "connection_state.h"

#include <sys/time.h>
#include <boost/shared_ptr.hpp>

class TcpdumpQueryEntry : public QueryEntry
{

  boost::shared_ptr<ConnectionState> connection_state;
  timeval       pcap_timestamp;
  std::string   query;

public:

  TcpdumpQueryEntry()
  {
    pcap_timestamp.tv_sec= pcap_timestamp.tv_usec= 0;
  }

  TcpdumpQueryEntry(boost::shared_ptr<ConnectionState> _connection_state,
		    const timeval       &_pcap_timestamp,
                    const std::string   &_query,
                    const AddrPort      &addr_port) :
    QueryEntry(addr_port.ThreadId(), false),
    connection_state(_connection_state),
    pcap_timestamp(_pcap_timestamp),
    query(_query)
  {}

  bool is_quit() { return false; }

  void execute(DBThread *t);
};

class TcpdumpResultEntry : public QueryEntry
{

  boost::shared_ptr<ConnectionState> connection_state;
  timeval       pcap_timestamp;
  QueryResult   expected_result;

public:
  TcpdumpResultEntry() {}
  TcpdumpResultEntry(boost::shared_ptr<ConnectionState> _connection_state,
		     const AddrPort     &addr_port,
                     const timeval      &_pcap_timestamp,
                     const QueryResult  &_expected_result) :
    QueryEntry(addr_port.ThreadId(), false),
    connection_state(_connection_state),
    pcap_timestamp(_pcap_timestamp),
    expected_result(_expected_result)
  {}

  bool is_quit() { return false; }

  void execute(DBThread *t);

};

#endif// PERCONA_PLAYBACK_TCPDUMP_QUERY_ENTRIES_H

