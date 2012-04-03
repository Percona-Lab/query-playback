dnl Copyright (C) 2010 Monty Taylor
dnl Copyright (C) 2011 Percona Inc
dnl This file is free software; Monty Taylor and Percona Inc
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_TBB],[
  AC_REQUIRE([AC_LIB_PREFIX])

  dnl --------------------------------------------------------------------
  dnl  Check for tbb
  dnl --------------------------------------------------------------------

  AC_ARG_ENABLE([tbb],
    [AS_HELP_STRING([--disable-tbb],
      [Build with Threading Building Blocks support @<:@default=on@:>@])],
    [ac_enable_tbb="$enableval"],
    [ac_enable_tbb="yes"])

  AS_IF([test "x$ac_enable_tbb" = "xyes"],[
    AC_LANG_PUSH(C++)
    AC_LIB_HAVE_LINKFLAGS(tbb,,
      [#include <tbb/tbb.h>],
      [tbb::atomic<uint64_t> queries],
      [system])
    AC_LANG_POP()
  ],[
    ac_cv_tbb="no"
  ])

  AM_CONDITIONAL(HAVE_TBB, [test "x${ac_cv_tbb}" = "xyes"])
  
])

AC_DEFUN([PANDORA_HAVE_TBB],[
  _PANDORA_SEARCH_TBB($1)
])

AC_DEFUN([PANDORA_REQUIRE_TBB],[
  PANDORA_HAVE_TBB($1)
  AS_IF([test x$ac_cv_tbb = xno],
      AC_MSG_ERROR([tbb is required for ${PACKAGE}]))
])

