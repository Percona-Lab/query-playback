========================
 About Percona Playback
========================

|Percona Playback| is a tool for replaying the load of one database server to another. Currently it can read queries from |MySQL| query-log and |tcpdump| files and run them on other |MySQL| server. It has plugin architecture and can be extended with different plugins.

There are four categories of plugins for |percona-playback|:

 * **input** - responsible for where input data is given from,
 * **db** - where queries should be played,
 * **report** - how to represent results,
 * **other** - plugins that doesn't belong to the previous categories.

Each plugin can have own set of command line options which are usually provided with help messages.

At this moment the following plugins are implemented:

 1) `input`

   * query_log - reads queries from query-log files
   * tcpdump - reads queries from tcpdump files

 2) `db`

   * libmysqlclient - plays queries in mysql server
   * null - doesn't play queries anywhere but useful for testing

 3) `report`

   * simple_report - output information about executed queries in simple form

The engine's architecture is "thread-per-connection". Each thread has queries queue. `Input` plugin parses input data and pass parsed queries to the engine. The engine pushes queries to the queue of certain `db` thread. The queue size can be limited with :option:`--queue-depth` command line option. If the limit is reached the engine will stop `input` plugin thread until the size of the queue becomes less then the limit.

.. image:: /_static/pp_intro.png

Input data can be played several times in a row. The number of repeats can be set with --loop command line options (NYI).
