=============================
 Installing Percona Playback
=============================

.. toctree::
   :hidden:

Compiling from Source
=====================

Build Dependencies
------------------
You will need the following packages to build percona-playback:
 * libtbb-dev (Intel Threading Building Blocks)
 * Boost (including boost program_options)
 * intltool
 * gettext (including gettext-devel on CentOS/RedHat)
 * libpcap-dev
 * libcloog-ppl0 (if using GCC 4.6)
 * libmysqlclient-dev (MySQL Client Libraries) [#n-1]_
 * libdrizzle-dev

Building
--------
If you are building from BZR, you will need to run :file:`./config/autorun.sh` first. :: 

  $ ./configure
  $ make

There are several tests included, these use a dummy database client plugin to mainly test that parsing of the logs is correct. ::

  $ make check

After that you can install the percona-playback with: :: 

  $ sudo make install

In some cases you'll still need to manually link the libraries

.. rubric:: Footnotes

.. [#n-1] Not strictly needed, but if you don't have it, you don't get to do the play back part.
