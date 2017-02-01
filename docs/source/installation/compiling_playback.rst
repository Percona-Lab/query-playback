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
  libboost-program-options-dev libboost-thread-dev libboost-regex-dev libboost-system-dev \
  pkg-config cmake

In ``RPM``-based distributions, you need to: ::

  # yum install autoconf automake libtool gettext-devel \
  libpcap-devel tbb-devel mysql mysql-devel intltool \
  boost-program-options-devel boost-thread-devel boost-regex-devel boost-system-devel \
  pkgconfig cmake

Package ``libmysqlclient-dev`` is not strictly needed for compiling, but if you don't have it, you don't get to do the play back part.

Building
--------

  $ mkdir build_dir
  $ cd build_dir
  $ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
  $ make
  
For a debug build run cmake with ``-DCMAKE_BUILD_TYPE=Debug`` and if you would like to compile the plugins as shared libraries use `-DBUILD_SHARED_LIBS:BOOL=ON`.


There are several tests included, these use a dummy database client plugin to mainly test that parsing of the logs is correct. ::

  $ make test

After that you can install the percona-playback with: :: 

  $ sudo make install

In some cases you'll still need to manually link the libraries.
