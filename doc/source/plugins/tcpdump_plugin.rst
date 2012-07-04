================
 tcpdump plugin
================

The main purpose of this plugin is to parse |MySQL| queries from |tcpdump| files. Currently this plugin doesn't support work with *prepare* and *execute* statements. Also it doesn't parse MySQL :option:`threads_id` because they are passed only during handshake, but tcpdump tool can be started in the middle of the session. That's why :option:`thread_id` which can be seen in the report of *report* plugins is some hash from client ip-port pair. Currently only parsing of ipv4 connections is implemented.

The plugin has two modes of work:
 * **accurate** - preserves queries execution time and pauses between queries, it's possible to playback the same load that was recorded on production with some accuracy.
 * **fast** - play queries as fast as possible.

The example of usage:
Playback percona_playback/test/tcpdump_accuracy.dump on MySQL server in *accurate* mode and queries queue limit of 10 000 elements: :: 

  $ bin/percona-playback --input-plugin=tcpdump --tcpdump-file=percona-playback/test/tcpdump_accuracy.dump \
   --tcpdump-mode=accurate --db-plugin=libmysqlclient --mysql-host=some_host --mysql-port=3307 \
   --mysql-username=test_user --mysql-password=passW0rd --mysql-schema=test1 --queue-depth 10000

