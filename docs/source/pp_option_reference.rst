===========================
 Playback Option Reference
===========================

This page documents the command-line options for the :program:`percona-playback`.

Options
=======

.. option:: --help

   This option displays a help screen and exits.

.. option:: --version

   This option displays the |percona-playback| version and copyright notice and then exits.

.. option:: --loop N

   Do the whole run N times.

Database Options:
-----------------
.. option:: --db-plugin=NAME       

   This options tells |percona-playback| which database plugin to use. 

.. option:: --dispatcher-plugin=DISPATCHER

   Dispatcher plugin used to replay queries.

.. option:: --input-plugin=NAME

   This options tells |percona-playback| which input plugin to use. 

.. option:: --queue-depth[=#]

  Queue depth for DB executor (thread). The larger this number is the greater the played-back workload can deviate from the original workload as some connections may be up to queue-depth behind. If no values is specified default is 1.

.. option:: --session-init-query

 This query will be executed just after each connect to database

MySQL Client Options:
---------------------
.. option::  --mysql-host=HOST

   Hostname of MySQL server

.. option::  --mysql-port=PORT

   MySQL port number

.. option:: --mysql-username=USERNAME

   Username to connect to MySQL
 
.. option::  --mysql-password=PASSWORD

   Password for MySQL user
 
.. option::  --mysql-schema=SCHEMA

   MySQL Schema to connect to

Query Log Options:
------------------
.. option::  --query-log-file=FILNAME

   Query log file that's going to be used.

.. option::  --query-log-read-count=[#]  

   Query log file read count (how many times to read query log file). If no value is specified default is 1.

.. option::  --query-log-set-timestamp        

   By default, query SET TIMESTAMP=XX; that the MySQL slow query log always includes is skipped. This may cause some subsequent queries to fail, depending on the workload. If the :option:`--run-set-timestamp` option is enabled, these queries are run as well.

.. option::  --query-log-preserve-query-time 

  This option ensures that each query takes at least Query_time (from slow query log) to execute.

Simple Report Options:
----------------------
.. option::  --show-per-connection-query-count

   Display the number of queries executed for each connection.

Threads-pool Options:
---------------------
.. option::  --thread-pool-threads-count=[#]

   The number of threads in thread pool. If this options is omitted the number of threads equals to hardware concurrency.

