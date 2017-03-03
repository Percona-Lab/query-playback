#ifndef PERCONA_PLAYBACK_GENERAL_LOG_ENTRY_H
#define PERCONA_PLAYBACK_GENERAL_LOG_ENTRY_H

#include <percona_playback/visibility.h>
#include "percona_playback/query_entry.h"
#include <percona_playback/db_thread.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <tbb/atomic.h>

#ifdef __cplusplus
extern "C"
{
#endif

class GeneralLogEntry : public QueryEntry
{
    private:
      uint64_t thread_id;
      uint64_t rows_sent;
      uint64_t rows_examined;
      double query_time;
      std::vector<std::string> info;
      std::string set_timestamp_query;
      std::string query;
    public:

      GeneralLogEntry() : thread_id(0), rows_sent(0), rows_examined(0), query_time(0) {}

      virtual uint64_t getThreadId() const { return thread_id; }

      inline double getQueryTime() { return query_time; }

      void add_query_line(const std::string &s);

      inline const std::string& getQuery() { return query; };

      inline void display() { std::cerr << "    " << query << std::endl; }

      inline bool is_quit() const { return (query.compare(0, 30, "# administrator command: Quit;") == 0); }

      void execute(DBThread *t);
};


#ifdef __cplusplus
}
#endif

#endif /* PERCONA_PLAYBACK_GENERAL_LOG_ENTRY_H */
