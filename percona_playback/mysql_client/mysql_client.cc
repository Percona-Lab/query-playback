

#include <percona_playback/mysql_client/mysql_client.h>

void MySQLDBThread::connect()
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

void MySQLDBThread::disconnect()
{
  mysql_close(&handle);
}

void MySQLDBThread::execute_query(const std::string &query)
{
  if(mysql_real_query(&handle, query.c_str(), query.length()) > 0)
  {
    // FIXME: error handling
  }
  else
  {
    MYSQL_RES* mysql_res= NULL;
    mysql_res= mysql_store_result(&handle);
    mysql_free_result(mysql_res);
  }
}
