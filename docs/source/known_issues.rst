============
Known Issues
============

There are a few known issues with :program:`percona-playback` that can
affect the expected result. Some of these are solvable through various
work arounds by the user, others only by changing :program:`percona-playback`
itself. We hope to somewhat adequately address all of these in future releases.

Connection Leaks in the Playback
=================================
Both the tcpdump plugin and query_log plugin suffer from what is known as connection leaks.

In the case of tcpdump plugin, :program:`percona-playback` reads the tcpdump file and creates new MySQL threads whenever it meets new connection creation or any data from new connection, and then terminates the connections when connection breaking is parsed from the tcpdump file. But there can be situations when not all tcp packets are in the tcpdump file (it can be due to additional CPU or network load when the tcpdump data was captured). In this case packets that signal connection termination are lost and :program:`percona-playback` will never kill MySQL thread which can lead to "too many connections" error on the MySQL server.

In the case of query_log plugin, when query_log parser meets new thread_id it creates new MySQL thread and sends queries using that thread. When there is a quit command in the query log file for a particular thread_id, then :program:`percona-playback` closes the MySQL connection and terminates the thread. But it can be the case when there are threads in query log without quit command may be due to abnormal session termination (connection break due to session inactivity for example). And this will again lead to connection leaks because such connections won't get closed.

Accurate Mode
=============

The option --query-log-preserve-query-time is analogue of tcpdump plugin accurate mode. The difference is tcpdump plugin accurate mode preserves both query execution time and delays between queries, but query_log plugin preserves only query execution time. So --query-log-preserve-query-time is not completely accurate. However, if the load on the host where the tcpdump is captured is high then that could lead to dropped packets which will make the tcpdump plugin less accurate.

Note also that accuracy is only with respect to queries executed within a single connection, for example suppose there are two MySQL threads, thread_id 1 and thread_id 2, then in that case accuracy will only deal with executing queries accurately within the context of the threads. So for example if the query log contains an entry first of a select by thread_id 1 and then an insert by thread_id 2, this order will not be guaranteed, and its possible that when :program:`percona-playback` replays the query log the insert by thread_id 2 is done before the select by thread_id 1. Therefore :program:`percona-playback` can only be used for load testing and cannot be used for functional testing as there is no concept of global accuracy.

Format of tcpdump capture
=========================

I think it should also be emphasized that its necessary to capture tcpdump in raw format, i.e. by using the -w option, if the capture is not done in raw format then :program:`percona-playback` cannot work with it. This is different for example from the type of tcpdump capture that :program:`pt-query-digest` expects.
