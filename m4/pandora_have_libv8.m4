dnl  Copyright (C) 2009 Sun Microsystems, Inc.
dnl This file is free software; Sun Microsystems, Inc.
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

#--------------------------------------------------------------------
# Check for libv8
#--------------------------------------------------------------------


AC_DEFUN([_PANDORA_SEARCH_LIBV8],[
  AC_REQUIRE([AC_LIB_PREFIX])

  # v8 is written in C++, need to use g++ for test link below
  AC_LANG_CPLUSPLUS

  AC_LIB_HAVE_LINKFLAGS(v8, pthread,
  [
    #include <v8.h>
  ],[
    v8::HandleScope handle_scope;
  ]) 

  AM_CONDITIONAL(HAVE_LIBV8, [test "x${ac_cv_libv8}" = "xyes"])
])

AC_DEFUN([_PANDORA_HAVE_LIBV8],[
  AC_ARG_ENABLE([libv8],
    [AS_HELP_STRING([--disable-libv8],
      [Build with libv8 support @<:@default=on@:>@])],
    [ac_enable_libv8="$enableval"],
    [ac_enable_libv8="yes"])

  _PANDORA_SEARCH_LIBV8
])


AC_DEFUN([PANDORA_HAVE_LIBV8],[
  AC_REQUIRE([_PANDORA_HAVE_LIBV8])
])

AC_DEFUN([_PANDORA_REQUIRE_LIBV8],[
  ac_enable_libv8="yes"
  _PANDORA_SEARCH_LIBV8

  AS_IF([test x$ac_cv_libv8 = xno],[
    PANDORA_MSG_ERROR([libv8 is required for ${PACKAGE}. On Debian this can be found in libv8-dev. On RedHat this can be found in libv8-devel.])
  ])
])

AC_DEFUN([PANDORA_REQUIRE_LIBV8],[
  AC_REQUIRE([_PANDORA_REQUIRE_LIBV8])
])
