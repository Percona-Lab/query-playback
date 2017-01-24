#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/smart_ptr.hpp>

#include <percona_playback/general_log/general_log_entry.h>

#include <percona_playback/general_log/parse_general_log.h>

void* ParseGeneralLog::operator() (void*)
{
    std::vector<boost::shared_ptr<GeneralLogEntry> > *entries = new std::vector<boost::shared_ptr<GeneralLogEntry> >();
    boost::shared_ptr<GeneralLogEntry> tmp_entry(new GeneralLogEntry());

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
        delete entries;
        return NULL;
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
                entries->push_back(tmp_entry);
                (*nr_queries)++;
                (*nr_entries)++;
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
    return entries;
}
