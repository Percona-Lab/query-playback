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
