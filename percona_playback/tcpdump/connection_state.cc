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
#include <pcap.h>
#include <iostream>
#ifdef HAVE_LIBDRIZZLE_1_0_DRIZZLE_CLIENT_H
#include <libdrizzle-1.0/drizzle_server.h>
#include <libdrizzle-1.0/row_client.h>
#else
#include <libdrizzle/drizzle_server.h>
#include <libdrizzle/row_client.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "connection_state.h"
#include "tcpdump_query_entries.h"
#include "percona_playback/plugin.h"

extern percona_playback::DBClientPlugin *g_dbclient_plugin;
extern percona_playback::DispatcherPlugin *g_dispatcher_plugin;

void
ConnectionState::ProcessMysqlPkts(boost::shared_ptr<ConnectionState> cs_ptr,
				  const u_char      *pkts,
                                  u_int             pkts_len,
                                  const timeval     &ts,
                                  const AddrPort    &addr_port,
                                  OUT TcpdumpMysqlParserStats  &stats)
{
  
  u_int used_len= 0;
  struct MysqlPkt *m;

  assert(pkts);

  // If last pkt was fragmented, merge with current pkts   
  if (fragmented) 
  {
    fragmented= false;
    size_t old_frag_buff_size= frag_buff.size();
    frag_buff.insert(frag_buff.end(), pkts, pkts + pkts_len);
    pkts= &frag_buff[0];
    pkts_len+= old_frag_buff_size;
  }

  for(;;)
  {
    m= (struct MysqlPkt *)pkts; // Packet header

    // Check if pkts > len of pkts actually received (last pkt is fragmented)
    used_len+= m->full_length();

    if (used_len > pkts_len) {
      fragmented= true;

      size_t frag_len= m->full_length() - (used_len - pkts_len);
                       
      UCharBuffer new_frag_buff(pkts, pkts + frag_len);
      frag_buff.swap(new_frag_buff);
      
      break;
    }

    if (m->data_length())
    {
      UCharBuffer buff(pkts, pkts + m->full_length());
      std::string query;
      PktResult retval= ParseMysqlPkt(buff, query);
      switch(retval)
      {
      case PKT_QUERY:
        if (!query.empty())
          DispatchQuery(cs_ptr, ts, query, addr_port);
        ++stats.nr_of_parsed_queries;
        ++stats.nr_of_parsed_packets;
        break;

      case PKT_RESULT:
        DispatchResult(cs_ptr, ts, addr_port);
        ++stats.nr_of_parsed_packets;
        break;

      case PKT_ERROR:
        ++stats.nr_of_parsing_errors;
        break;

      default:
        ++stats.nr_of_parsed_packets;
      }
    }
    
    pkts += m->full_length(); // Next pkt header

    if (used_len == pkts_len)
      break;

  }

  if (!fragmented)
    frag_buff.clear();

}

PktResult
ConnectionState::ParseMysqlPkt(IN UCharBuffer  &buff,
                               OUT std::string &query)
{
  if (current_origin == CLIENT)
    return ClientPacket(buff, query);
  
  if (current_origin == SERVER && was_query)
    return ServerPacket(buff);

  return PKT_UNKNOWN;
}

PktResult
ConnectionState::ServerPacket(IN UCharBuffer &buff)
{
  PktResult retval= PKT_OK;

  drizzle_return_t ret;

  drizzle_con->buffer_ptr= &buff[0];
  drizzle_con->buffer_size= buff.size();

  /*
    Check if we should continue parsing result set or
    we can start a new loop of result parsing.
  */
  if (!drizzle_con->result)
  {
  /*  
    If there are not any parsed results then it can be "Ok",
    "Error" or "Results set header" packets.
  */
    last_query_result.clear();

    drizzle_result_st *res= drizzle_result_read(drizzle_con.get(),
                                                NULL,
                                                &ret);
    /* Check if the packet is "Error" packet */
    if (ret == DRIZZLE_RETURN_ERROR_CODE)
    {
      last_query_result.setError(drizzle_result_error_code(res));
      retval= PKT_RESULT;
      /*
        The text representation of error can be got with
        drizzle_result_error(res)
      */
      goto free_and_return;
    }
    else if(ret == DRIZZLE_RETURN_OK)
    {
      /* Check if the packet is "Ok" packet */
      if (!drizzle_result_column_count(res))
      {
        last_query_result.setRowsSent(drizzle_result_row_count(res));
        last_query_result.setRowsExamined(drizzle_result_affected_rows(res));
        last_query_result.setWarningCount(drizzle_result_warning_count(res));
        retval= PKT_RESULT;
        goto free_and_return;
      }
      /* Else this is "Result set header" packet */
    }
    else
    {
      std::cerr << "Unknown packet type from server" << std::endl;
      retval= PKT_ERROR;
      goto free_and_return;
    }
  }
  /* Parse result set */
  else
  {
    /* "Field" packets */
    if (eof_count == 0)
    {
      drizzle_column_st *column= drizzle_column_read(drizzle_con->result,
                                                     NULL,
                                                     &ret);
      if (ret == DRIZZLE_RETURN_OK)
      {
        /* "EOF" packet */
        if (column == NULL)
          ++eof_count;
        retval= PKT_OK;
      }
      else
      {
        std::cerr << "Parse column error" << std::endl;
        retval= PKT_ERROR;
        goto free_and_return;
      }
    }
    /* "Row" packets */
    else if (eof_count == 1)
    {
      uint8_t  old_packet_number= drizzle_con->packet_number;
      uint64_t row= drizzle_row_read(drizzle_con->result, &ret);
      if (ret != DRIZZLE_RETURN_OK)
      {
        std::cerr << "Error in parsing row" << std::endl;
        retval= PKT_ERROR;
        goto free_and_return;
      }
      else
        ++sent_rows_count;
      /* "EOF" packet */
      if (!row)
      {
        eof_count= 0;
      
        drizzle_con->buffer_ptr= &buff[0];
        drizzle_con->buffer_size= buff.size();
   
        drizzle_result_free_all(drizzle_con.get());
        drizzle_con->packet_number= old_packet_number;
        drizzle_result_st *res= drizzle_result_read(drizzle_con.get(),
                                                    NULL,
                                                    &ret);
        /*
          Setting DRIZZLE_RESULT_ALLOCATED in result options
          helps to avoid memory leak.
          See https://bugs.launchpad.net/drizzle/+bug/1015576
        */
        res->options=
          (drizzle_result_options_t)
            ((uint64_t)(res->options) | (uint64_t)DRIZZLE_RESULT_ALLOCATED);
        last_query_result.setWarningCount(drizzle_result_warning_count(res));
        /* Don't count the last EOF packet */
        last_query_result.setRowsSent(sent_rows_count - 1);

        sent_rows_count= 0;
        retval= PKT_RESULT;
        goto free_and_return;
      }
      else
        retval= PKT_OK;
    }
  }

  return retval;

free_and_return:
  drizzle_result_free_all(drizzle_con.get());
  drizzle_con->result= NULL;
  was_query= false;
  return retval;
}

PktResult
ConnectionState::ClientPacket(IN UCharBuffer   &buff,
                              OUT std::string   &query)
{
  drizzle_command_t command;
  drizzle_return_t ret;
  size_t total;
  uint8_t *data;
  PktResult result= PKT_UNKNOWN;

  drizzle_con->buffer_ptr= &buff[0];
  drizzle_con->buffer_size= buff.size();
  data= (uint8_t *)drizzle_con_command_buffer(drizzle_con.get(),
                                              &command,
                                              &total,
                                              &ret);
  if (ret == DRIZZLE_RETURN_OK)
  {
    switch(command)
    {
      case DRIZZLE_COMMAND_QUERY:
        query.assign((const char *)data);
        was_query= true;
        result= PKT_QUERY;
        break;

      case DRIZZLE_COMMAND_INIT_DB:
        query.assign("use ");
        query+= (const char *)data;
        was_query= true;
        result= PKT_QUERY;
        break;

      default:;
    }
  }

  free(data);
  return result;
}

void
ConnectionState::DispatchQuery(/* Having shared pointer to "this" as an argument
				  is just temporary step. We need a shared pointer
				  to connection state inside of query entry. The
				  better way to do this is to move code that creates
				  query entries to the upper layer.
			       */
			       boost::shared_ptr<ConnectionState> cs_ptr,
			       const timeval        &ts,
                               const std::string    &query,
                               const AddrPort       &addr_port)
{
  boost::shared_ptr<TcpdumpQueryEntry> 
    query_entry(new TcpdumpQueryEntry(cs_ptr,
				      ts,
                                      query,
                                      addr_port));
  g_dispatcher_plugin->dispatch(query_entry);
}

void
ConnectionState::DispatchResult(boost::shared_ptr<ConnectionState> cs_ptr,
				const timeval    &ts,
                                const AddrPort   &addr_port)
{
  boost::shared_ptr<TcpdumpResultEntry> 
    result_entry(new TcpdumpResultEntry(cs_ptr, addr_port, ts, last_query_result));
  g_dispatcher_plugin->dispatch(result_entry);
}

