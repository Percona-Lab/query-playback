
#include <percona_playback/db_thread.h>
#include <mysql.h>

class QueryResult;
class MySQLOptions;

class MySQLDBThread : public DBThread
{
 private:
  MYSQL handle;
  MySQLOptions *options;
  static const unsigned max_err_num = 10;

 public:
  MySQLDBThread(uint64_t _thread_id, MySQLOptions *opt) :
    DBThread(_thread_id,
	     boost::shared_ptr<Queries>(new Queries())),
    options(opt)
  {
  }

  bool connect();
  void disconnect();
  void execute_query(const std::string &query, QueryResult *r,
		     const QueryResult &expected_result);
  void run();
};
