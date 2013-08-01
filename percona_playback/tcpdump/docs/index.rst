================
 tcpdump plugin
================

The main purpose of this plugin is to parse |MySQL| queries from |tcpdump| files. Currently this plugin doesn't support work with *prepare* and *execute* statements. Also it doesn't parse MySQL :option:`threads_id` because they are passed only during handshake, but tcpdump tool can be started in the middle of the session. That's why :option:`thread_id` which can be seen in the report of *report* plugins is some hash from client ip-port pair. Currently only parsing of ipv4 connections is implemented.

The plugin has two modes of work:
 * **accurate** - preserves queries execution time and pauses between queries, it's possible to playback the same load that was recorded on production with some accuracy.
 * **fast** - play queries as fast as possible.

.. note::
 The |tcpdump| plugin can parse packets only on raw format. The "-w" option must be used for |tcpdump| during capturing to get input files for the tool. 

Example
=======

1) Queries will be captured with :program:`tcpdump` to the :file:`example.dump`: 
 
.. code-block:: bash
 
  $ tcpdump -i eth0 port 3306 -w example.dump

.. note::

   |Percona Playback| currently doesn't support ``any`` as and tcpdump interface option. If "-i any" is used for capturing tcpdump parser works wrong.

2) |Percona Playback| is started with tcpdump plugin reading the example.dump file, connecting to remote |MySQL| server in *accurate* mode: 

.. code-block:: bash

  $ percona-playback --input-plugin=tcpdump --tcpdump-file=example.dump --tcpdump-mode=accurate \
  --db-plugin=libmysqlclient --mysql-host=10.8.2.10 --mysql-username=root \
  --mysql-password=passW0rd --mysql-schema=imdb

3) After the |Percona Playback| is done, report is generated that looks like this:  

.. code-block:: bash

  Report
  ------
  Executed 22 queries
  Spent 00:00:32.844442 executing queries versus an expected 00:00:00.503753 time.
  1 queries were quicker than expected, 21 were slower
  A total of 0 queries had errors.
  Expected 30298 rows, got 30298 (a difference of 0)
  Number of queries where number of rows differed: 0.

  Average of 22.00 queries per connection (1 connections).

