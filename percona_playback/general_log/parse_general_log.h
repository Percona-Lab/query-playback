#ifndef PERCONA_PLAYBACK_PARSE_GENERAL_LOG
#define PERCONA_PLAYBACK_PARSE_GENERAL_LOG

#include <iostream>
#include <stdlib.h>

#include <tbb/pipeline.h>
#include <tbb/tick_count.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/tbb_allocator.h>
#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_hash_map.h>

#include <percona_playback/plugin.h>

#ifdef __cplusplus
extern "C"
{
#endif

class ParseGeneralLog {
    public:
      ParseGeneralLog(FILE *input_file_,
                unsigned int run_count_)
        : input_file(input_file_),
          run_count(run_count_),
          next_line(NULL),
          next_len(0)
      {};

      boost::shared_ptr<QueryEntries> getEntries();

    private:
      FILE *input_file;
      unsigned int run_count;
      char *next_line;
      ssize_t next_len;
};

#ifdef __cplusplus
}
#endif

#endif // PERCONA_PLAYBACK_PARSE_GENERAL_LOG
