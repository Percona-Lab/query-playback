==================================
 |Percona| Playback release notes
==================================

Percona Playback 0.3
====================
Add basic tcpdump playback and basic documentation.
Build and test works on increased number of platforms.

Percona Playback 0.2
====================
Added --preserve-query-time to ensure that each query takes at least Query_time (from slow query log) to execute by injecting a usleep() call afterwards if needed.

Percona Playback 0.1
====================
Initial public release. Just supports reading from slow query log and blasting queries at a database server.

Features:
 * slow query log parsing
 * mysql client library
 * one thread per client thread
 * basic report as to how the replay went
 * "as fast as possible" playback only. If query execution takes less time on playback server than original one, percona-playback will not pause.

Notable missing features:
 * wall-time playback: ability to have the same amount of real-world time pass (i.e. a gap of 10seconds between queries will be preserved).
 * parsing of general query log
