============
Known Issues
============

There are a few known issues with :program:`percona-playback` that can
affect the expected result. Some of these are solvable through various
work arounds by the user, others only by changing :program:`percona-playback`
itself. We hope to somewhat adequately address all of these in future releases.

Connection Leaks in the Playback
=================================
The query_log plugin suffer from what is known as connection leaks.

In the case of query_log plugin, when query_log parser meets new thread_id it creates new MySQL thread and sends queries using that thread. When there is a quit command in the query log file for a particular thread_id, then :program:`percona-playback` closes the MySQL connection and terminates the thread. But it can be the case when there are threads in query log without quit command may be due to abnormal session termination (connection break due to session inactivity for example). And this will again lead to connection leaks because such connections won't get closed.

Accurate Mode
=============

The option --query-log-preserve-query-time is enables accurate mode.  Accurate mode does not preserve delays in between queries.  The query_log plugin preserves only query execution time. So --query-log-preserve-query-time is not completely accurate. 

Note also that accuracy is only with respect to queries executed within a single connection, for example suppose there are two MySQL threads, thread_id 1 and thread_id 2, then in that case accuracy will only deal with executing queries accurately within the context of the threads. So for example if the query log contains an entry first of a select by thread_id 1 and then an insert by thread_id 2, this order will not be guaranteed, and its possible that when :program:`percona-playback` replays the query log the insert by thread_id 2 is done before the select by thread_id 1. Therefore :program:`percona-playback` can only be used for load testing and cannot be used for functional testing as there is no concept of global accuracy.
