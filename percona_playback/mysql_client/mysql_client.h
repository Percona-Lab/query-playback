
#include <percona_playback/db_thread.h>

class QueryResult;
class MySQLOptions;

class MySQLDBThread : public DBThread
{
 private:
  MYSQL handle;
  MySQLOptions *options;

 public:
  MySQLDBThread(uint64_t _thread_id, MySQLOptions *opt) : DBThread(_thread_id) {
  }

  void connect();
  void disconnect();
  void execute_query(const std::string &query, QueryResult *r);
};
