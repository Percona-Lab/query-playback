/* BEGIN LICENSE
 * Copyright (C) 2011 Percona Inc.
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

#include "config.h"

#include <stdlib.h>
#include <string>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <assert.h>
#include "boost/thread.hpp"
#include "query_log.h"
#include <unistd.h>

#include "tbb/pipeline.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tbb_allocator.h"
#include "tbb/atomic.h"
#include "tbb/concurrent_queue.h"
#include "tbb/concurrent_hash_map.h"

#include <mysql/mysql.h>

//! Holds a slice of text.
/** Instances *must* be allocated/freed using methods herein, because the C++ declaration
    represents only the header of a much larger object in memory. */
class TextSlice {
    //! Pointer to one past last character in sequence
    char* logical_end;
    //! Pointer to one past last available byte in sequence.
    char* physical_end;
public:
    //! Allocate a TextSlice object that can hold up to max_size characters.
    static TextSlice* allocate( size_t max_size ) {
        // +1 leaves room for a terminating null character.
      TextSlice* t = reinterpret_cast<TextSlice*>(tbb::tbb_allocator<char>().allocate( sizeof(TextSlice)+max_size+1 ));
        t->logical_end = t->begin();
        t->physical_end = t->begin()+max_size;
        return t;
    }
    //! Free a TextSlice object
    void free() {
      tbb::tbb_allocator<char>().deallocate(reinterpret_cast<char*>(this),
				  sizeof(TextSlice)+(physical_end-begin())+1);
    }
    //! Pointer to beginning of sequence
    char* begin() {return (char*)(this+1);}
    //! Pointer to one past last character in sequence
    char* end() {return logical_end;}
    //! Length of sequence
    size_t size() const {return logical_end-(char*)(this+1);}
    //! Maximum number of characters that can be appended to sequence
    size_t avail() const {return physical_end-logical_end;}
    //! Append sequence [first,last) to this sequence.
    void append( char* first, char* last ) {
        memcpy( logical_end, first, last-first );
        logical_end += last-first;
    }
    //! Set end() to given value.
    void set_end( char* p ) {logical_end=p;}
};

const size_t MAX_CHAR_PER_INPUT_SLICE = 4000;

class LineFileInputFilter : public tbb::filter {
public:
  LineFileInputFilter( FILE* input_file_, unsigned int read_count_);
  ~LineFileInputFilter();
  void* operator()(void*);
private:
  FILE* input_file;
  TextSlice* next_slice;
  unsigned int read_count;
};

LineFileInputFilter::LineFileInputFilter( FILE* input_file_, unsigned int read_count_ ) :
    input_file(input_file_),
    next_slice( NULL ),
    read_count(read_count_),
    tbb::filter(/*serial*/true)
{
}

LineFileInputFilter::~LineFileInputFilter() {
  if (next_slice)
    next_slice->free();
}

void* LineFileInputFilter::operator()(void*) {
  if (!next_slice)
    next_slice= TextSlice::allocate( MAX_CHAR_PER_INPUT_SLICE );

  // Read characters into space that is available in the next slice.
  size_t m = next_slice->avail();
 read:
  size_t n = fread( next_slice->end(), 1, m, input_file );
  if( !n && next_slice->size()==0 && read_count)
  {
    fseek(input_file, 0L, SEEK_SET);
    read_count--;
    goto read;
  }

  if( !n && next_slice->size()==0 ) {
    // No more characters to process
    return NULL;
  } else {
    // Have more characters to process.
    TextSlice& t = *next_slice;
    next_slice = TextSlice::allocate( MAX_CHAR_PER_INPUT_SLICE );
    char* p = t.end()+n;
    if( n==m ) {
      // Might have read partial number.  If so, transfer characters of partial number to next slice.
      while( p>t.begin() && p[-1]!='\n' )
	--p;
      next_slice->append( p, t.end()+n );
    }
    t.set_end(p);
    return &t;
  }
}

class QueryLogEntry {
private:
  uint64_t thread_id;
  std::vector<std::string> info;
  std::vector<std::string> query;
  bool shutdown;
public:

  QueryLogEntry() : thread_id(0), shutdown(false) {};

  void set_shutdown() { shutdown= true; }

  bool is_shutdown() { return shutdown; }

  uint64_t getThreadId() { return thread_id; }

  void add_line(const std::string &s, tbb::atomic<uint64_t> *queries)
  {
    size_t location= s.find("Thread_id: ");
    if (location != std::string::npos)
    {
      thread_id = strtoull(s.c_str() + location + strlen("Thread_Id: "), NULL, 10);
    }
    if (s[0] == '#' && strncmp(s.c_str(), "# administrator", strlen("# administrator")))
      info.push_back(s);
    else
    {
      query.push_back(s);
      (*queries)++;
    }
  }

  void display()
  {
    std::vector<std::string>::iterator it;

    for ( it=query.begin() ; it < query.end(); it++ )
      std::cerr << "    " << *it << std::endl;
  }

  bool is_quit()
  {
    return query.size() &&
      (query[0].compare("# administrator command: Quit;") == 0
       || (query.size()>1 && query[1].compare("# administrator command: Quit;") == 0));
  }

  bool execute(MYSQL* handle)
  {
    std::vector<std::string>::iterator it;

    for ( it=query.begin() ; it < query.end(); it++ )
    {
      const std::string timestamp_query("SET timestamp=");
      if((*it).compare(0, timestamp_query.length(), timestamp_query) == 0)
	continue;
      /*      std::cerr << "thread " << getThreadId()
	      << " running query " << (*it) << std::endl;*/
      if(mysql_real_query(handle, (*it).c_str(), (*it).length()) > 0)
      {
	// FIXME: error handling
      }
      else
      {
	MYSQL_RES* mysql_res= NULL;
	mysql_res= mysql_store_result(handle);
	mysql_free_result(mysql_res);
      }
    }
  }
};

// ParseQueryLogFunc
class ParseQueryLogFunc: public tbb::filter {
public:
  ParseQueryLogFunc(tbb::atomic<uint64_t> *entries_,
		    tbb::atomic<uint64_t> *queries_)
    : previous(NULL),
      entries(entries_),
      queries(queries_),
      tbb::filter(true)
  {};

  ~ParseQueryLogFunc() { delete previous; }

  void* operator() (void*);
private:
  QueryLogEntry *previous;
  tbb::atomic<uint64_t> *entries;
  tbb::atomic<uint64_t> *queries;
};

void* dispatch(void *input_);

void* ParseQueryLogFunc::operator() (void* input_)  {
  TextSlice *input= static_cast<TextSlice*>(input_);

  std::vector<QueryLogEntry> *entries = new std::vector<QueryLogEntry>();

  if(previous)
    entries->push_back(*previous);
  delete previous;

  entries->push_back(QueryLogEntry());

  char *p= input->begin();
  char *q= input->begin();

  for (;;) {
    while (q<input->end() && *q != '\n')
      q++;

    if(q==input->end())
      break;

    if ( (strncmp(p, "# Time", 5) == 0))
      goto next;

    if (p[0] != '#' && (q-p) >= strlen("started with:")
	&& strncmp(q- strlen("started with:"), "started with:", strlen("started with:"))==0)
      goto next;

    if (p[0] != '#' && strncmp(p, "Tcp port: ", strlen("Tcp port: "))==0)
      goto next;

    if (p[0] != '#' && strncmp(p, "Time    ", strlen("Time    "))==0)
      goto next;

    if (strncmp(p, "# User@Host", strlen("# User@Host")) == 0)
    {
      entries->push_back(QueryLogEntry());
      (*this->entries)++;
    }

    entries->back().add_line(std::string(p, q - p), queries);
  next:
    p= q+1;
    q++;
  }

  this->previous = new QueryLogEntry(entries->back());
  entries->pop_back();

  input->free();
  return entries;
}

class DBThread;
void RunDBThread(DBThread* dbt, uint64_t thread_id);

class DBThread {
private:
  MYSQL handle;
  boost::thread *thread;
  uint64_t thread_id;
public:
  tbb::concurrent_bounded_queue<QueryLogEntry> queries;

  DBThread(uint64_t _thread_id) : thread_id(_thread_id) {
    queries.set_capacity(1);
  }

  ~DBThread() {
    delete thread;
  }

  void join()
  {
    thread->join();
  }

  void connect()
  {
    mysql_init(&handle);
    mysql_real_connect(&handle,
		       "127.0.0.1",
		       "root",
		       "",
		       "test",
		       13010,
		       "",
		       0);
  }

  void disconnect()
  {
    mysql_close(&handle);
  }

  bool run()
  {
    connect();

    QueryLogEntry query;
    while (true)
    {
      queries.pop(query);
      if (query.is_shutdown())
      {
	disconnect();
	return true;
      }
      if (query.is_quit())
	/*	if(queries.empty())
	  break;
	  else*/
	{ disconnect(); connect(); continue;}

      query.execute(&handle);
    }

    disconnect();
    return false;
  }

  void start_thread()
  {
    thread= new boost::thread(RunDBThread, this, thread_id);
  }
};

typedef tbb::concurrent_hash_map<uint64_t, DBThread*> DBExecutorsTable;

DBExecutorsTable db_executors;

void RunDBThread(DBThread* dbt, uint64_t thread_id)
{
  if (dbt->run() == false)
  {
    db_executors.erase(thread_id);
    delete dbt;
  }
  std::cerr << "end thread " << thread_id << std::endl;
}

void* dispatch (void *input_)
{
    std::vector<QueryLogEntry> *input= static_cast<std::vector<QueryLogEntry>*>(input_);
    for (int i=0; i< input->size(); i++)
    {
      //      usleep(10);
      uint64_t thread_id= (*input)[i].getThreadId();
      {
	DBExecutorsTable::accessor a;
	if (db_executors.insert( a, thread_id ))
	{
	  DBThread *db_thread= new DBThread(thread_id);
	  a->second= db_thread;
	  db_thread->start_thread();
	  std::cerr << "new thread " << thread_id << std::endl;
	}
	a->second->queries.push((*input)[i]);
      }
    }
    delete input;
    return NULL;
}

// DispatchQueriesFunc
class DispatchQueriesFunc : public tbb::filter {
public:
  DispatchQueriesFunc() : tbb::filter(true) {};

  void* operator() (void *input_)
  {
    return dispatch(input_);
  }
};


/*
 * Read in lines of file
 * find item boundaries (in order)
 * parse item (parallel)
 * dispatch (in order)
 */

void LogReaderThread(FILE* input_file, unsigned int run_count)
{
  tbb::pipeline p;
  tbb::atomic<uint64_t> entries;
  tbb::atomic<uint64_t> queries;
  entries=0;
  queries=0;

  LineFileInputFilter f1(input_file, run_count);
  ParseQueryLogFunc f2(&entries, &queries);
  DispatchQueriesFunc f4;
  p.add_filter(f1);
  p.add_filter(f2);
  p.add_filter(f4);
  p.run(2);

  QueryLogEntry shutdown_command;
  shutdown_command.set_shutdown();

  while(db_executors.size())
  {
    uint64_t thread_id;
    DBThread *t;
    {
      DBExecutorsTable::const_iterator iter= db_executors.begin();
      thread_id= (*iter).first;
      t= (*iter).second;
    }
    db_executors.erase(thread_id);
    t->queries.push(shutdown_command);
    t->join();
    delete t;
  }

  std::cout << "Processed " << entries << " entries" << std::endl;
  std::cout << "Processed " << queries << " queries" << std::endl;
}

int run_query_log(const std::string &log_file, unsigned int run_count)
{
  FILE* input_file = fopen(log_file.c_str(),"r");
  if (input_file == NULL)
    return -1;

  boost::thread log_reader_thread(LogReaderThread,input_file, run_count);

  log_reader_thread.join();
  fclose(input_file);

  return 0;
}
