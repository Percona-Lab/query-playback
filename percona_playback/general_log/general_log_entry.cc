#include "config.h"

#include <stdlib.h>
#include <string>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>

#include <percona_playback/percona_playback.h>
#include <percona_playback/plugin.h>

#include <percona_playback/general_log/general_log_entry.h>
#include <percona_playback/query_result.h>

#include <boost/foreach.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>


void GeneralLogEntry::execute(DBThread *t)
{
  std::vector<std::string>::iterator it;
  QueryResult r;

  QueryResult expected_result;
  expected_result.setRowsSent(rows_sent);
  expected_result.setRowsExamined(rows_examined);
  expected_result.setError(0);

  boost::posix_time::time_duration expected_duration = boost::posix_time::microseconds(long(query_time * 1000000));
  expected_result.setDuration(expected_duration);

  boost::posix_time::ptime start_time;
  start_time= boost::posix_time::microsec_clock::universal_time();

  t->execute_query(query, &r, expected_result);

  boost::posix_time::ptime end_time;
  end_time= boost::posix_time::microsec_clock::universal_time();

  boost::posix_time::time_period duration(start_time, end_time);
  r.setDuration(duration.length());

  BOOST_FOREACH(const percona_playback::PluginRegistry::ReportPluginPair pp, percona_playback::PluginRegistry::singleton().report_plugins)
  {
    if (pp.second->active)
      pp.second->query_execution(getThreadId(), query, expected_result, r);
  }
}

void GeneralLogEntry::add_query_line(const std::string &s)
{
    boost::regex re("\\s+(\\d+)\\s+Query\\s+(.+)");
    boost::smatch fields;    //std::cout << "LINE [" << s << "]" << std::endl;

    if (boost::regex_search(s, fields, re))
    {
        //0 whole string
        //1 Thread id
        //2 query        std::string ns = fields[2].str();        std::string::const_iterator begin = ns.begin();        std::string::const_iterator end = ns.end() - 1;
        if (ns.length() >= 2 && *(ns.end() - 2) == '\r')
            --end;        //std::cout << "MATCHING THREADID [" << fields[1] << "] QUERY [" << fields[2] << "]" << std::endl;
        thread_id = strtoull(fields[1].str().c_str(), NULL, 10);
        query.append(begin, end);
        query.append(" ");
    }
}


