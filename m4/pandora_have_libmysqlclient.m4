dnl -*- mode: m4; c-basic-offset: 2; indent-tabs-mode: nil; -*-
dnl vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
dnl
dnl  Copyright (C) 2010 Monty Taylor
dnl  This file is free software; Sun Microsystems
dnl  gives unlimited permission to copy and/or distribute it,
dnl  with or without modifications, as long as this notice is preserved.
dnl

AC_DEFUN([PANDORA_WITH_MYSQL],[
  AC_ARG_WITH([mysql],
    [AS_HELP_STRING([--with-mysql=PATH],
        [path to mysql_config binary or mysql prefix dir])], 
      [with_mysql=$withval],
      [with_mysql=":"])

  dnl There are three possibilities:
  dnl   1) nothing is given: we will search for mysql_config in PATH
  dnl   2) the location of mysql_config is given: we'll use that to determine
  dnl   3) a directory argument is given: that will be mysql_base

     
  dnl option 1: nothing, we need to insert something into MYSQL_CONFIG
  AS_IF([test "x$with_mysql" = "x:"],[
    AC_CHECK_PROGS(MYSQL_CONFIG,[mysql_config])
  ],[
    MYSQL_CONFIG="${with_mysql}"
  ])

  AC_CACHE_CHECK([for MySQL Base Location],[pandora_cv_mysql_base],[

    dnl option 2: something in MYSQL_CONFIG now, use that to get a base dir
    AS_IF([test -f "`which ${MYSQL_CONFIG}`" -a -x "`which ${MYSQL_CONFIG}`"],[
      pandora_cv_mysql_base=$(dirname $(${MYSQL_CONFIG} --include | sed 's/-I//'))
      MYSQL_INCLUDES=$(${MYSQL_CONFIG} --include)
      MYSQL_INCLUDES="$MYSQL_INCLUDES $(echo $MYSQL_INCLUDES|sed -e 's/-I/-isystem /')"
      MYSQL_LIBS=$(${MYSQL_CONFIG} --libs_r)
    ],[
      dnl option 1: a directory
      AS_IF([test -d $with_mysql],
            [
              pandora_cv_mysql_base=$with_mysql
              IBASE="-I${with_mysql}"
              MYSQL_CONFIG="${with_mysql}/bin/mysql_config"
              MYSQL_INCLUDES="$IBASE/include/mysql -isystem $IBASE/include/mysql"
              MYSQL_LIBS="-L${with_mysql}/lib -L${with_mysql}/lib/mysql -lmysqlclient_r"
            ],
            [
              pandora_cv_mysql_base=""
      ])
    ])
  ])
    AC_SUBST(MYSQL_CONFIG)
    AC_SUBST(MYSQL_INCLUDES)
    AC_SUBST(MYSQL_LIBS)
])

AC_DEFUN([_PANDORA_SEARCH_LIBMYSQLCLIENT],[
  AC_REQUIRE([AC_LIB_PREFIX])

  AC_ARG_ENABLE([libmysqlclient],
    [AS_HELP_STRING([--disable-libmysqlclient],
      [Build with libmysqlclient support @<:@default=on@:>@])],
    [ac_enable_libmysqlclient="$enableval"],
    [ac_enable_libmysqlclient="yes"])

  AS_IF([test "x$ac_enable_libmysqlclient" = "xyes"],[
    AC_LIB_HAVE_LINKFLAGS(mysqlclient_r,,[
#include <mysql/mysql.h>
    ],[
MYSQL mysql;
  ])],[
    ac_cv_libmysqlclient_r="no"
  ])

  AM_CONDITIONAL(HAVE_LIBMYSQLCLIENT, [test "x${ac_cv_libmysqlclient_r}" = "xyes"])

  AS_IF([test "x$MYSQL_CONFIG" = "xISDIR"],[
    IBASE="-I${with_mysql}"
    MYSQL_CONFIG="${with_mysql}/scripts/mysql_config"
    ADDIFLAGS="$IBASE/include -isystem $IBASE/include"
    ADDLDFLAGS="-L${with_mysql}/libmysql_r/.libs/ -L${with_mysql}/mysys/.libs -L${with_mysql}/mysys -L${with_mysql}/strings/.libs -L${with_mysql}/strings "
  ],[
    MYSQL_INCLUDES=$(${MYSQL_CONFIG} --include)
    MYSQL_INCLUDES="$MYSQL_INCLUDES $(echo $MYSQL_INCLUDES|sed -e 's/-I/-isystem /')"
    MYSQL_LIBS=$(${MYSQL_CONFIG} --libs_r)
  ])

    AC_SUBST(MYSQL_CONFIG)
    AC_SUBST(MYSQL_INCLUDES)
    AC_SUBST(MYSQL_LIBS)
])

AC_DEFUN([PANDORA_HAVE_LIBMYSQLCLIENT],[
  AC_REQUIRE([_PANDORA_SEARCH_LIBMYSQLCLIENT])
])

AC_DEFUN([PANDORA_REQUIRE_LIBMYSQLCLIENT],[
  AC_REQUIRE([PANDORA_HAVE_LIBMYSQLCLIENT])
  AS_IF([test "x${ac_cv_libmysqlclient_r}" = "xno"],
      PANDORA_MSG_ERROR([libmysqlclient_r is required for ${PACKAGE}]))
])

