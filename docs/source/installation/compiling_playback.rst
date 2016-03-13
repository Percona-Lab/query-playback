===================================================
Compiling and Installing Playback from Source Code
===================================================

The source code is available from the *Github* project `here <https://github.com/percona/percona-playback>`_. The easiest way to get the code is with :command:`git clone` of the desired release, such as the following: ::
 
  git clone https://github.com/percona/percona-playback

You should then have a directory named after the release you branched, such as ``percona-playback``.


Compiling on Linux
==================

Prerequisites
-------------

The following packages and tools must be installed to compile *Playback* from source. These might vary from system to system.

In Debian-based distributions, you need to: ::

  # apt-get install debhelper autoconf automake libtool \
  gettext intltool libpcap-dev libtbb-dev libmysqlclient-dev \ 
  libboost-program-options-dev libboost-thread-dev pkg-config

In ``RPM``-based distributions, you need to: ::

  # yum install autoconf automake libtool gettext-devel \
  libpcap-devel tbb-devel mysql mysql-devel intltool \
  boost-program-options-devel boost-thread-devel pkgconfig

Package ``libmysqlclient-dev`` is not strictly needed for compiling, but if you don't have it, you don't get to do the play back part.

Building
--------
If you are building from git, you will need to run :file:`./config/autorun.sh` first. :: 

  $ ./configure
  $ make

There are several tests included, these use a dummy database client plugin to mainly test that parsing of the logs is correct. ::

  $ make check

After that you can install the percona-playback with: :: 

  $ sudo make install

In some cases you'll still need to manually link the libraries.
