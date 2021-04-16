Query Playback
----------------
© 2011-2017 Percona LLC.
© 2017 Dropbox, Inc.
See [LICENSE](LICENSE) for full text of the GPL.

Query Playback is a tool for replaying the load of one database server to another.

see `./bin/percona-playback --help` for details

Docker
------------------

Build docker:   `docker build -t replay -f Dockerfile ../`

Run docker:   `docker run -it --entrypoint=/bin/bash --name replay -d replay`

Login docker:  `docker exec -it replay bash`

Build Dependencies
------------------
Query Playback is mostly comprised of plugins, some of which have their
own build dependencies (and simply won't be built if you don't have the
required libraries).

You will need the 'development' packages for all of these (often suffixed by
'-devel' for RedHat or '-dev' for Debian derived distributions).

The core of Query Playback requires:

 * libtbb (Intel Threading Building Blocks)
 * Boost >= 1.53
 * pkg-config
 * cmake >= 2.8.7

The plugin that uses the MySQL client library needs:
 * libmysqlclient (MySQL Client Libraries)

Install dependencies (CentOS 7)
-------------------------------

    yum -y install tbb tbb-devel cmake boost boost-devel

Building
--------

    $ mkdir build_dir
    $ cd build_dir
    $ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
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

