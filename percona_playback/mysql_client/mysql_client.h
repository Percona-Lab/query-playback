
#include <percona_playback/db_thread.h>

class MySQLDBThread : public DBThread
{
 private:
  MYSQL handle;

 public:
  MySQLDBThread(uint64_t _thread_id) : DBThread(_thread_id) {
  }

  void connect();
  void disconnect();
  void execute_query(const std::string &query);
};
