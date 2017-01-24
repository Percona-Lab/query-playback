#include "config.h"

#include <stdlib.h>
#include <string>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <vector>

#include <percona_playback/visibility.h>
#include <percona_playback/percona_playback.h>
#include <percona_playback/plugin.h>
#include <percona_playback/general_log/general_log.h>
#include <percona_playback/general_log/general_log_entry.h>
#include <percona_playback/general_log/parse_general_log.h>
#include <percona_playback/gettext.h>

#include <boost/program_options.hpp>
#include <boost/thread.hpp>


#include <tbb/pipeline.h>
#include <tbb/tick_count.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/tbb_allocator.h>
#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_hash_map.h>


extern percona_playback::DBClientPlugin *g_dbclient_plugin;
extern percona_playback::DispatcherPlugin *g_dispatcher_plugin;

class DispatchGeneralQueries : public tbb::filter {
    public:
        DispatchGeneralQueries() : tbb::filter(true) {};

        void* operator() (void *input_)
        {
            std::vector<boost::shared_ptr<GeneralLogEntry> > *input = static_cast<std::vector<boost::shared_ptr<GeneralLogEntry> >*>(input_);
            for (unsigned int i=0; i< input->size(); i++)
            {
              g_dispatcher_plugin->dispatch((*input)[i]);
            }
            delete input;
            return NULL;
        }
};

static void GeneralLogReaderThread(FILE* input_file, unsigned int run_count, struct percona_playback_run_result *r)
{
  tbb::pipeline p;
  tbb::atomic<uint64_t> entries;
  tbb::atomic<uint64_t> queries;
  entries = 0;
  queries = 0;

  ParseGeneralLog f2(input_file, run_count, &entries, &queries);
  DispatchGeneralQueries f4;
  p.add_filter(f2);
  p.add_filter(f4);
  p.run(2);

  g_dispatcher_plugin->finish_all_and_wait();

  r->n_log_entries = entries;
  r->n_queries = queries;
}


PERCONA_PLAYBACK_API
int run_query_log(const std::string &log_file, unsigned int read_count, struct percona_playback_run_result *r);

GeneralLogPlugin::GeneralLogPlugin(const std::string &_name) :
    InputPlugin(_name),
    options(_("General Log Options")),
    read_count(1),
    std_in(false)
{
};

boost::program_options::options_description* GeneralLogPlugin::getProgramOptions() {
    options.add_options()
        ("general-log-file", po::value<std::string>(), _("General log file"))
        ("general-log-stdin", po::value<bool>()->default_value(false)->zero_tokens(), _("Read general log from stdin"));

        return &options;
}

  int GeneralLogPlugin::processOptions(boost::program_options::variables_map &vm)
  {
    if (!active && (vm.count("general-log-file") || !vm["general-log-stdin"].defaulted()))
    {
      fprintf(stderr,_(("general-log plugin is not selected, you shouldn't use this plugin-related command line options\n")));
      return -1;
    }

    if (!active)
      return 0;

    if (vm.count("general-log-file") && vm["general-log-stdin"].as<bool>())
    {
      fprintf(stderr,  _(("The options --general-log-file and --general-log-stdin can not be used together\n")));
      return -1;
    }

    if (vm.count("general-log-file"))
      file_name= vm["general-log-file"].as<std::string>();
    else if (vm["general-log-stdin"].as<bool>())
    {
      std_in = true;
    }
    else
    {
      fprintf(stderr, _("ERROR: --general-log-file is a required option.\n"));
      return -1;
    }

    return 0;
  }

  void GeneralLogPlugin::run(percona_playback_run_result  &result)
  {
    FILE* input_file;

    if (std_in)
    {
      input_file = stdin;
    }
    else
    {
      input_file = fopen(file_name.c_str(),"r");
      if (input_file == NULL)
      {
        fprintf(stderr,
		_("ERROR: Error opening file '%s': %s"),
		file_name.c_str(), strerror(errno));
	return;
      }
    }

    boost::thread log_reader_thread(GeneralLogReaderThread, input_file, read_count, &result);

    log_reader_thread.join();
    fclose(input_file);
  }

static void init(percona_playback::PluginRegistry&r)
{
  r.add("general-log", new GeneralLogPlugin("general-log"));
}

PERCONA_PLAYBACK_PLUGIN(init);
