#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>
#include <boost/unordered_set.hpp>

#include <percona_playback/general_log/general_log_entry.h>

#include <percona_playback/general_log/parse_general_log.h>

class GeneralLogQueryEntries : public QueryEntries {
public:
  typedef std::vector<boost::shared_ptr<GeneralLogEntry> > Entries;
  Entries entries;
  Entries::size_type pos;

  GeneralLogQueryEntries() : pos(0) {}

  virtual boost::shared_ptr<QueryEntry> popEntry() {
    boost::shared_ptr<QueryEntry> entry;
    if (pos < entries.size()) {
      entry = entries[pos++];
    }
    return entry;
  }

  virtual void setShutdownOnLastQueryOfConn() {
    // automatically close threads after last request
    boost::unordered_set<uint64_t> thread_ids;
    for (Entries::reverse_iterator it = entries.rbegin(), end = entries.rend(); it != end; ++it) {
      if (thread_ids.insert((*it)->getThreadId()).second)
        (*it)->set_shutdown();
    }
  }
};

boost::shared_ptr<QueryEntries> ParseGeneralLog::getEntries()
{
    boost::shared_ptr<GeneralLogQueryEntries> entries = boost::make_shared<GeneralLogQueryEntries>();
    boost::shared_ptr<GeneralLogEntry> tmp_entry = boost::make_shared<GeneralLogEntry>();

    char *line= NULL;
    size_t buflen = 0;
    ssize_t len;

    if (next_line)
    {
        line = next_line;
        len = next_len;
        next_line = NULL;
        next_len = 0;
    }
    else if ((len = getline(&line, &buflen, input_file)) == -1)
    {
        return entries;
    }

    char *p = line;
    char *q;
    bool ignoreLine = false;

    do
    {
        q = line + len;
        ignoreLine = false;

        if ( p[0] == '#' ||
            (p[0] != '#' && (
                (( (q - p) >= (ssize_t)strlen("started with:\n")) && strncmp(q - strlen("started with:\n"), "started with:", strlen("started with:")) == 0) ||
                (strncmp(p, "Tcp port: ", strlen("Tcp port: ")) == 0) ||
                (strncmp(p, "Time Id Command Argument", strlen("Time Id Command Argument")) == 0)
            )))
        {
            ignoreLine = true;
        }

        if (!ignoreLine)
        {
            tmp_entry->add_query_line(std::string(line));
            if (!tmp_entry->getQuery().empty())
            {
                entries->entries.push_back(tmp_entry);
                tmp_entry.reset(new GeneralLogEntry());
            }
        }

        if (!next_line && ((len = getline(&line, &buflen, input_file)) == -1))
        {
            break;
        }

        next_line = NULL;
        p = line;
    } while(true);

    free(line);

    entries->setNumEntries(entries->entries.size());
    entries->setNumQueries(entries->entries.size());

    return entries;
}
