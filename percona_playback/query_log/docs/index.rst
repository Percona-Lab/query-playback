==================
 Query_log plugin
==================

This plugin parses queries from query log files. It can preserve query execution time with :option:`--query-log-preserve-query-time` option. The :option:`--query-log-read-count` options allows to replay query log file several times (NYI). The difference between this and :option:`--loop` options should be that :option:`--loop` reports at the end of each execution whereas :option:`--query-log-read-count` reports once after all executions.

The example of usage:
Run percona_playback/test/basic-slow.log on default libmysqlplugin settings: ::

 $ percona_playback --db-plugin=libmysqlclient --query-log-file=./percona_playback/test/basic-slow.log

The other options description can be found in "help" message.
