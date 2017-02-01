Query Playback
----------------
© 2011-2016 Percona Inc.
See [LICENSE](LICENSE) for full text of the GPL.

Query Playback is a tool for replaying the load of one database server to another.

see `./bin/percona-playback --help` for details

Build Dependencies
------------------
Query Playback is mostly comprised of plugins, some of which have their
own build dependencies (and simply won't be built if you don't have the
required libraries).

You will need the 'development' packages for all of these (often suffixed by
'-devel' for RedHat or '-dev' for Debian derived distributions).

The core of Query Playback requires:

 * libtbb (Intel Threading Building Blocks)
 * Boost (including boost `program_options`) at least 1.41
 * libtool
 * gettext
 * intltool
 * pkg-config
 * cmake

The plugin that uses the MySQL client library needs:
 * libmysqlclient (MySQL Client Libraries)

Building
--------

    $ mkdir build_dir
    $ cd build_dir
    $ cmake ..
    $ make

Test Suite
----------
There are several tests included, these use a dummy database client plugin to mainly test that parsing of the logs is correct.

    $ make check

Building release tarball
------------------------

    $ cpack ..

Building release debian package:
------------------------

    $ cpack -G DEB ..

Capturing everything in the slow log
------------------------------------
To capture queries for Query Playback to play back, you will need to run MySQL, MariaDB or Percona Server with the slow query log capturing the queries you wish to run. You will need the following options to mysqld:

    --slow-query-log --log-slow-admin-statements --log-slow-verbosity=microtime --long-query-time=0

Documentation
-------------

You can build the documentation by running make in the docs directory.

