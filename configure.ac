#
# Percona Playback
#
# Portions of auto-foo:
# Copyright (C) 2009-2010 Monty Taylor, Eric Day
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.
#

AC_INIT([percona-playback],[0.5],[https://bugs.launchpad.net/percona-playback])
AC_CONFIG_SRCDIR([percona_playback/percona_playback.h])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])

PANDORA_CANONICAL_TARGET(warnings-always-on, require-cxx)

# Increment if interfaces have been added, removed, or changed.
LIBPERCONA_PLAYBACK_VERSION_CURRENT=1

# Increment if source code has changed, set to zero if current is incremented.
LIBPERCONA_PLAYBACK_VERSION_REVISION=0

# Increment if interfaces have been added, set to zero if interfaces have been
# removed or changed.
LIBPERCONA_PLAYBACK_VERSION_AGE=0

AC_SUBST(LIBPERCONA_PLAYBACK_VERSION_CURRENT)
AC_SUBST(LIBPERCONA_PLAYBACK_VERSION_REVISION)
AC_SUBST(LIBPERCONA_PLAYBACK_VERSION_AGE)

AC_DEFINE_UNQUOTED([LIBPERCONA_PLAYBACK_VERSION_CURRENT],
                   [$LIBPERCONA_PLAYBACK_VERSION_CURRENT],
                   [Current portion of libpercona_playback version])
AC_DEFINE_UNQUOTED([LIBPERCONA_PLAYBACK_VERSION_REVISION],
                   [$LIBPERCONA_PLAYBACK_VERSION_REVISION],
                   [Revision portion of libpercona_playback version])
AC_DEFINE_UNQUOTED([LIBPERCONA_PLAYBACK_VERSION_AGE],
                   [$LIBPERCONA_PLAYBACK_VERSION_AGE],
                   [Age portion of libpercona_playback version])

LIBPERCONA_PLAYBACK_VERSION=$LIBPERCONA_PLAYBACK_VERSION_CURRENT:$LIBPERCONA_PLAYBACK_VERSION_REVISION:$LIBPERCONA_PLAYBACK_VERSION_AGE

AC_SUBST(LIBPERCONA_PLAYBACK_VERSION)

AC_LANG_PUSH(C++)
PANDORA_REQUIRE_PTHREAD
PANDORA_REQUIRE_LIBDL
AC_LANG_POP

PANDORA_REQUIRE_BOOST
PANDORA_REQUIRE_BOOST_PROGRAM_OPTIONS
PANDORA_REQUIRE_BOOST_THREAD
PANDORA_REQUIRE_TBB

PANDORA_PLUGINS([percona_playback/plugin_load_list.h])

AC_DEFINE_UNQUOTED([PERCONA_PLAYBACK_MODULE_EXT], ["$acl_cv_shlibext"],
                   [Extension to use for modules.])
AM_CXXFLAGS="${AM_CXXFLAGS} ${NO_EFF_CXX} ${NO_OLD_STYLE_CAST} ${NO_VISIBILITY}"
AC_CONFIG_FILES(Makefile dnl po/Makefile.in
 percona_playback/version.h dnl
 doxygen/header.html)

AC_OUTPUT

echo "---"
echo "Configuration summary for $PACKAGE_NAME version $VERSION"
echo ""
echo "   * Installation prefix:       $prefix"
echo "   * System type:               $host_vendor-$host_os"
echo "   * Host CPU:                  $host_cpu"
echo "   * C Compiler:                $CC_VERSION"
echo "   * Assertions enabled:        $ac_cv_assert"
echo "   * Debug enabled:             $with_debug"
echo "   * Profiling enabled:         $ac_profiling"
echo "   * Coverage enabled:          $ac_coverage"
echo "   * Warnings as errors:        $ac_cv_warnings_as_errors"
echo "   * Default CFLAGS:            $AM_CFLAGS"
echo "   * Default CXXFLAGS:          $AM_CXXFLAGS"
echo "   * Default LDFLAGS:           $AM_LDFLAGS"
echo "   * Builtin modules:"
for module in `echo $PANDORA_BUILTIN_LIST|tr ',' '\n' |sort`
do
  echo "     - $module"
done
echo ""
echo "   * Dynamic modules:"
for module in `echo $PANDORA_PLUGIN_LIST | tr ',' '\n' | sort`
do
  echo "     - $module"
done

echo ""
echo "---"

dnl libtoolize scans configure.ac  and needs to see some text
m4_define([LIBTOOLIZE_AC_INIT], [])
m4_define([UNUSED_MACRO], [
  LT_INIT
  AC_CONFIG_MACRO_DIR([m4])
])

