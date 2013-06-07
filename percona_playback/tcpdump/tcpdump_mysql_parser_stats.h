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

#ifndef PERCONA_PLAYBACK_TCPDUMP_MYSQL_PARSER_STATS_H
#define PERCONA_PLAYBACK_TCPDUMP_MYSQL_PARSER_STATS_H

struct TcpdumpMysqlParserStats
{
  size_t            nr_of_parsed_packets;
  size_t            nr_of_parsed_queries;
  size_t            nr_of_parsing_errors;
  
  TcpdumpMysqlParserStats() :
    nr_of_parsed_packets(0),
    nr_of_parsed_queries(0),
    nr_of_parsing_errors(0)
  {}

};

#endif //PERCONA_PLAYBACK_TCPDUMP_MYSQL_PARSER_STATS_H
