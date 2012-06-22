#
# Percona Playback
#
### BEGIN LICENSE
# Copyright (C) 2011 Stewart Smith <stewart@flamingspork.com>
# This program is free software: you can redistribute it and/or modify it 
# under the terms of the GNU General Public License version 3, as published 
# by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranties of 
# MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
# PURPOSE.  See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along 
# with this program.  If not, see <http://www.gnu.org/licenses/>.
### END LICENSE

ACLOCAL_AMFLAGS = -I m4
LDADD = libpercona_playback.la


bin_PROGRAMS =
check_PROGRAMS =
noinst_PROGRAMS =
lib_LTLIBRARIES =
noinst_LTLIBRARIES =
nobase_include_HEADERS =
noinst_HEADERS =
pkgplugin_LTLIBRARIES =
CLEANFILES =
DISTCLEANFILES = \
  config/top.h
EXTRA_DIST = \
  LICENSE \
  README \
  percona_playback/test/basic-slow.log \
  percona_playback/test/crashme-slow.log \
  percona_playback/test/sqlbench-transactions-slow.log \
  percona_playback/test/sysbench-slow.log \
  percona_playback/test/preserve_query_time.log \
  test_run.sh

include config/pandora-plugin.am

nobase_dist_pkginclude_HEADERS= percona_playback/version.h
#
# libpercona_playback
#

lib_LTLIBRARIES += libpercona_playback.la

libpercona_playback_la_SOURCES = \
  percona_playback/percona_playback.cc \
  percona_playback/plugin.cc \
  percona_playback/db_thread.cc

nobase_include_HEADERS += \
  percona_playback/percona_playback.h \
  percona_playback/db_thread.h \
  percona_playback/query_result.h \
  percona_playback/plugin.h \
  percona_playback/tokenize.h \
  percona_playback/visibility.h \
  percona_playback/gettext.h \
  percona_playback/query_entry.h

libpercona_playback_la_CXXFLAGS = \
	$(AM_CXXFLAGS) \
	-DBUILDING_PERCONA_PLAYBACK

libpercona_playback_la_LIBADD = $(LIBDL_LIBS) $(BOOST_LIBS) $(TBB_LIBS)
libpercona_playback_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(LIBPERCONA_PLAYBACK_VERSION) $(BOOST_LIBS) $(TBB_LIBS) $(pandora_plugin_libs)
libpercona_playback_la_DEPENDENCIES = ${noinst_LTLIBRARIES} $(pandora_plugin_libs)

check_PROGRAMS += \
  percona_playback/test/basic \
  percona_playback/test/basic-queue-depth \
  percona_playback/test/crashme-slow \
  percona_playback/test/sqlbench-transactions-slow \
  percona_playback/test/sysbench-slow \
  percona_playback/test/preserve_query_time \
  percona_playback/test/help

percona_playback_test_basic_CXXFLAGS= $(AM_CXXFLAGS) -DSRCDIR=\"${srcdir}\"
percona_playback_test_basic_queue_depth_CXXFLAGS= $(AM_CXXFLAGS) -DSRCDIR=\"${srcdir}\"
percona_playback_test_crashme_slow_CXXFLAGS= $(AM_CXXFLAGS) -DSRCDIR=\"${srcdir}\"
percona_playback_test_sqlbench_transactions_slow_CXXFLAGS= $(AM_CXXFLAGS) -DSRCDIR=\"${srcdir}\"
percona_playback_test_sysbench_slow_CXXFLAGS= $(AM_CXXFLAGS) -DSRCDIR=\"${srcdir}\"
percona_playback_test_preserve_query_time_CXXFLAGS= $(AM_CXXFLAGS) -DSRCDIR=\"${srcdir}\"

percona_playback_test_help_SOURCES= percona_playback/test/help.cc
percona_playback_test_help_LDADD= $(LDADD) $(MYSQL_LIBS)

percona_playback_test_basic_SOURCES= percona_playback/test/basic.cc
percona_playback_test_basic_LDADD= $(LDADD) $(MYSQL_LIBS)

percona_playback_test_basic_queue_depth_SOURCES= percona_playback/test/basic-queue-depth.cc
percona_playback_test_basic_queue_depth_LDADD= $(LDADD) $(MYSQL_LIBS)

percona_playback_test_crashme_slow_SOURCES= percona_playback/test/crashme-slow.cc
percona_playback_test_crashme_slow_LDADD= $(LDADD) $(MYSQL_LIBS)

percona_playback_test_sqlbench_transactions_slow_SOURCES= percona_playback/test/sqlbench-transactions-slow.cc
percona_playback_test_sqlbench_transactions_slow_LDADD= $(LDADD) $(MYSQL_LIBS)

percona_playback_test_sysbench_slow_SOURCES= percona_playback/test/sysbench-slow.cc
percona_playback_test_sysbench_slow_LDADD= $(LDADD) $(MYSQL_LIBS)

percona_playback_test_preserve_query_time_SOURCES= percona_playback/test/preserve_query_time.cc
percona_playback_test_preserve_query_time_LDADD= $(LDADD) $(MYSQL_LIBS)



#
# Simple percona_playback application
#

bin_PROGRAMS += bin/percona_playback
bin_percona_playback_SOURCES = bin/percona_playback.cc
bin_percona_playback_LDADD= $(BOOST_LIBS) $(LDADD) $(TBB_LIBS) $(MYSQL_LIBS) $(pandora_plugin_libs) $(PANDORA_DYNAMIC_LDADDS)

include docs/include.am

TESTS = $(check_PROGRAMS)

TESTS_ENVIRONMENT = ${top_srcdir}/test_run.sh

check-verbose:
	@PERCONA_PLAYBACK_TEST_ARGS="v" make check

check-debug:
	@PERCONA_PLAYBACK_TEST_ARGS="vvv" make check

check-valgrind:
	@PERCONA_PLAYBACK_TEST_PREFIX="valgrind -q --leak-check=full --show-reachable=yes --suppressions=valgrind.suppressions" make check

check-valgrind-debug:
	@PERCONA_PLAYBACK_TEST_PREFIX="valgrind -q --leak-check=full --show-reachable=yes --suppressions=valgrind.suppressions" PERCONA_PLAYBACK_TEST_ARGS="vvv" make check

check-gdb:
	@PERCONA_PLAYBACK_TEST_PREFIX="gdb -q" make check

check-time:
	@PERCONA_PLAYBACK_TEST_PREFIX="/usr/bin/time" make check

check-strace:
	@PERCONA_PLAYBACK_TEST_PREFIX="strace -c" make check

check-truss:
	@PERCONA_PLAYBACK_TEST_PREFIX="truss -c" make check

SUBDIRS = po
