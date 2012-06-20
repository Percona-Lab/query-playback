dnl Macros that test for specific features.
dnl This file is part of the Autoconf packaging for Wireshark.
dnl Copyright (C) 1998-2000 by Gerald Combs.
dnl
dnl $Id$
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
dnl 02111-1307, USA.
dnl
dnl As a special exception, the Free Software Foundation gives unlimited
dnl permission to copy, distribute and modify the configure scripts that
dnl are the output of Autoconf.  You need not follow the terms of the GNU
dnl General Public License when using or distributing such scripts, even
dnl though portions of the text of Autoconf appear in them.  The GNU
dnl General Public License (GPL) does govern all other use of the material
dnl that constitutes the Autoconf program.
dnl
dnl Certain portions of the Autoconf source text are designed to be copied
dnl (in certain cases, depending on the input) into the output of
dnl Autoconf.  We call these the "data" portions.  The rest of the Autoconf
dnl source text consists of comments plus executable code that decides which
dnl of the data portions to output in any given case.  We call these
dnl comments and executable code the "non-data" portions.  Autoconf never
dnl copies any of the non-data portions into its output.
dnl
dnl This special exception to the GPL applies to versions of Autoconf
dnl released by the Free Software Foundation.  When you make and
dnl distribute a modified version of Autoconf, you may extend this special
dnl exception to the GPL to apply to your modified version as well, *unless*
dnl your modified version has the potential to copy into its output some
dnl of the text that was the non-data portion of the version that you started
dnl with.  (In other words, unless your change moves or copies text from
dnl the non-data portions to the data portions.)  If your modification has
dnl such potential, you must delete any notice of this special exception
dnl to the GPL from your modified version.
dnl
dnl Written by David MacKenzie, with help from
dnl Franc,ois Pinard, Karl Berry, Richard Pixley, Ian Lance Taylor,
dnl Roland McGrath, Noah Friedman, david d zuhn, and many others.

#
# PANDORA_BREAKLOOP_TRY_LINK
#
AC_DEFUN([PANDORA_PCAP_BREAKLOOP_TRY_LINK],
[
  AC_LINK_IFELSE(
  [
      AC_LANG_SOURCE(
      [[
#	include <pcap.h>
	int main(void)
	{
	  pcap_t  *pct = NULL;
	  pcap_breakloop(pct);
	  return 0;
	}
      ]])
  ],
  [
    ws_breakloop_compiled=yes
  ],
  [
    ws_breakloop_compiled=no
  ])
])
#
# PANDORA_ADD_DASH_L
#
# Add to the variable specified as the first argument a "-L" flag for the
# directory specified as the second argument, and, on Solaris, add a
# "-R" flag for it as well.
#
# XXX - IRIX, and other OSes, may require some flag equivalent to
# "-R" here.
#
AC_DEFUN([PANDORA_ADD_DASH_L],
[$1="$$1 -L$2"
case "$host_os" in
  solaris*)
    $1="$$1 -R$2"
  ;;
esac
])


#
# PANDORA_PCAP_CHECK
#
AC_DEFUN([PANDORA_HAVE_LIBPCAP],
[
        AC_MSG_CHECKING(libpcap)

        AC_ARG_WITH([libpcap],
          [AS_HELP_STRING([--with-libpcap=PATH],
          [path to libpcap prefix dir])], 
          [pcap_dir=$withval],
          )

	if test -z "$pcap_dir"
	then
	  # Pcap header checks
	  # XXX need to set a var AC_CHECK_HEADER(pcap.h,,)

	  #
	  # The user didn't specify a directory in which libpcap resides.
	  # First, look for a pcap-config script.
	  #
	  AC_PATH_PROG(PCAP_CONFIG, pcap-config)
        else
          if test -f $pcap_dir/bin/pcap-config -a -x $pcap_dir/bin/pcap-config
          then
            PCAP_CONFIG=$pcap_dir/bin/pcap-config
          fi
        fi

        if test -n "$PCAP_CONFIG" ; then
        #
        # Found it.
        #
        # Now check whether it's the libpcap 1.0 version, which
        # put a space after "-L" - on some platforms, that doesn't
        # work.
        #
          AC_MSG_CHECKING(for broken pcap-config)
          case `"$PCAP_CONFIG" --libs` in

          "-L "*)
            #
            # Space after -L.  Pretend pcap-config doesn't exist.
            #
            AC_MSG_RESULT(yes)
            PCAP_CONFIG=""
            ;;

          *)
            #
            # No space after -L.
            #
            AC_MSG_RESULT(no)
            ;;
          esac
        fi
        if test -n "$PCAP_CONFIG" ; then
          #
          # Found it, and it's usable; use it to get the include flags
          # for libpcap.
          #
          PCAP_INCLUDES=`"$PCAP_CONFIG" --cflags`
        else
          if test -d "$pcap_dir"
          then
  	    PCAP_INCLUDES="-I$pcap_dir/include"
	    PANDORA_ADD_DASH_L(PCAP_LIBS, $pcap_dir/lib)
          else
            #
            # Didn't find it; we have to look for libpcap ourselves.
            # We assume that the current library search path will work,
            # but we may have to look for the header in a "pcap"
            # subdirectory of "/usr/include" or "/usr/local/include",
            # as some systems apparently put "pcap.h" in a "pcap"
            # subdirectory, and we also check "$prefix/include" - and
            # "$prefix/include/pcap", in case $prefix is set to
            # "/usr/include" or "/usr/local/include".
            #
            # XXX - should we just add "$prefix/include" to the include
            # search path and "$prefix/lib" to the library search path?
            #
            AC_MSG_CHECKING(for extraneous pcap header directories)
            found_pcap_dir=""
            pcap_dir_list="/usr/include/pcap $prefix/include/pcap $prefix/include"
            if test "x$ac_cv_enable_usr_local" = "xyes" ; then
              pcap_dir_list="$pcap_dir_list /usr/local/include/pcap"
            fi
            for pcap_dir in $pcap_dir_list
            do
              if test -d $pcap_dir ; then
                if test x$pcap_dir != x/usr/include -a x$pcap_dir != x/usr/local/include ; then
                    PCAP_INCLUDES="-I$pcap_dir"
                fi
                found_pcap_dir=" $found_pcap_dir -I$pcap_dir"
                break
              fi
            done

            if test "$found_pcap_dir" != "" ; then
              AC_MSG_RESULT(found --$found_pcap_dir added to CFLAGS)
            else
              AC_MSG_RESULT(not found)
            fi
          fi
        fi

        ac_CFLAGS_saved=$CFLAGS
        CFLAGS="$PCAP_INCLUDES $CFLAGS"
        ac_CPPFLAGS_saved=$CPPFLAGS
        CPPFLAGS="$PCAP_INCLUDES $CPPFLAGS"

	# Pcap header check
	AC_CHECK_HEADER(pcap.h,,
	    AC_MSG_ERROR([[Header file pcap.h not found; if you installed libpcap
from source, did you also do \"make install-incl\", and if you installed a
binary package of libpcap, is there also a developer's package of libpcap,
and did you also install that package?]]))


	if test -n "$PCAP_CONFIG" ; then
	  #
	  # We have pcap-config; we assume that means we have libpcap
	  # installed and that pcap-config will tell us whatever
	  # libraries libpcap needs.
	  #
	  if test x$enable_static = xyes; then
	    PCAP_LIBS=`"$PCAP_CONFIG" --libs --static`
	  else
	    PCAP_LIBS=`"$PCAP_CONFIG" --libs`
	  fi
	  AC_DEFINE(HAVE_LIBPCAP, 1, [Define to use libpcap library])
	else
	  #
	  # Check to see if we find "pcap_open_live" in "-lpcap".
	  # Also check for various additional libraries that libpcap might
	  # require.
	  #
          ac_LDFLAGS_saved=$LDFLAGS
          LDFLAGS="-L${pcap_dir}/lib"
	  AC_CHECK_LIB(pcap, pcap_open_live,
	    [
	      PCAP_LIBS="-L${pcap_dir}/lib -lpcap"
	      AC_DEFINE(HAVE_LIBPCAP, 1, [Define to use libpcap library])
	    ], [
	      ac_wireshark_extras_found=no
	      ac_save_LIBS="$LIBS"
	      for extras in "-lcfg -lodm" "-lpfring"
	      do
		AC_MSG_CHECKING([for pcap_open_live in -lpcap with $extras])
		LIBS="-lpcap $extras"
		#
		# XXX - can't we use AC_CHECK_LIB here?
		#
		AC_TRY_LINK(
		    [
#	include <pcap.h>
		    ],
		    [
	pcap_open_live(NULL, 0, 0, 0, NULL);
		    ],
		    [
			ac_wireshark_extras_found=yes
			AC_MSG_RESULT([yes])
			PCAP_LIBS="-lpcap $extras"
			AC_DEFINE(HAVE_LIBPCAP, 1, [Define to use libpcap library])
		    ],
		    [
			AC_MSG_RESULT([no])
		    ])
		if test x$ac_wireshark_extras_found = xyes
		then
		    break
		fi
	      done
	      if test x$ac_wireshark_extras_found = xno
	      then
		AC_MSG_ERROR([Can't link with library libpcap.])
	      fi
	      LIBS=$ac_save_LIBS
	    ])
            LDFLAGS=$ac_LDFLAGS_saved
	fi

	#
	# Check whether various variables and functions are defined by
	# libpcap.
	#
	ac_save_LIBS="$LIBS"
	LIBS="$PCAP_LIBS $LIBS"
	AC_CHECK_FUNCS(pcap_open_dead pcap_freecode)
	#
	# pcap_breakloop may be present in the library but not declared
	# in the pcap.h header file.  If it's not declared in the header
	# file, attempts to use it will get warnings, and, if we're
	# building with warnings treated as errors, that warning will
	# cause compilation to fail.
	#
	# We are therefore first testing whether the function is present
	# and then, if we're compiling with warnings as errors, testing
	# whether it is usable.  It is usable if it compiles without
	# a -Wimplicit warning (the "compile with warnings as errors"
	# option requires GCC). If it is not usable, we fail and tell
	# the user that the pcap.h header needs to be updated.
	#
	# Ceteris paribus, this should only happen with Mac OS X 10.3[.x] which
	# can have an up-to-date pcap library without the corresponding pcap
	# header.
	#
	# However, it might also happen on some others OSes with some erroneous
	# system manipulations where multiple versions of libcap might co-exist
	# e.g. hand made symbolic link from libpcap.so -> libpcap.so.0.8 but
	# having the pcap header version 0.7.
	#
	AC_MSG_CHECKING([whether pcap_breakloop is present])
        CFLAGS="$PCAP_INCLUDES $CFLAGS"
	PANDORA_PCAP_BREAKLOOP_TRY_LINK
	if test "x$ws_breakloop_compiled" = "xyes"; then
	  AC_MSG_RESULT(yes)
	  AC_DEFINE(HAVE_PCAP_BREAKLOOP, 1, [Define if pcap_breakloop is known])
	  if test "x$with_warnings_as_errors" = "xyes"; then
	    AC_MSG_CHECKING([whether pcap_breakloop is usable])
	    CFLAGS="$CFLAGS -Werror -Wimplicit"
	    PANDORA_PCAP_BREAKLOOP_TRY_LINK
	    if test "x$ws_breakloop_compiled" = "xyes"; then
	      AC_MSG_RESULT(yes)
	    else
	      AC_MSG_RESULT(no)
	      AC_MSG_ERROR(
[Your pcap library is more recent than your pcap header.
As you are building with compiler warnings treated as errors, Wireshark
won't be able to use functions not declared in that header.
If you wish to build with compiler warnings treated as errors, You should
install a newer version of the header file.])
	    fi
	  fi
	else
	  AC_MSG_RESULT(no)
	fi

	#
	# Later versions of Mac OS X 10.3[.x] ship a pcap.h that
	# doesn't define pcap_if_t but ship an 0.8[.x] libpcap,
	# so the library has "pcap_findalldevs()", but pcap.h
	# doesn't define "pcap_if_t" so you can't actually *use*
	# "pcap_findalldevs()".
	#
	# That even appears to be true of systems shipped with
	# 10.3.4, so it doesn't appear only to be a case of
	# Software Update not updating header files.
	#
	# (You can work around this by installing the 0.8 header
	# files.)
	#
	AC_CACHE_CHECK([whether pcap_findalldevs is present and usable],
	  [ac_cv_func_pcap_findalldevs],
	  [
	    AC_LINK_IFELSE(
	      [
		AC_LANG_SOURCE(
		  [[
		    #include <pcap.h>
		    main()
		    {
		      pcap_if_t *devpointer;
		      char errbuf[1];

		      pcap_findalldevs(&devpointer, errbuf);
		    }
		  ]])
	      ],
	      [
		ac_cv_func_pcap_findalldevs=yes
	      ],
	      [
		ac_cv_func_pcap_findalldevs=no
	      ])
	  ])
	#
	# Don't check for other new routines that showed up after
	# "pcap_findalldevs()" if we don't have a usable
	# "pcap_findalldevs()", so we don't end up using them if the
	# "pcap.h" is crufty and old and doesn't declare them.
	#
	if test $ac_cv_func_pcap_findalldevs = "yes" ; then
	  AC_DEFINE(HAVE_PCAP_FINDALLDEVS, 1,
	   [Define to 1 if you have the `pcap_findalldevs' function and a pcap.h that declares pcap_if_t.])
	  AC_CHECK_FUNCS(pcap_datalink_val_to_name pcap_datalink_name_to_val)
	  AC_CHECK_FUNCS(pcap_datalink_val_to_description)
	  AC_CHECK_FUNCS(pcap_list_datalinks pcap_set_datalink pcap_lib_version)
	  AC_CHECK_FUNCS(pcap_get_selectable_fd pcap_free_datalinks)
	  AC_CHECK_FUNCS(pcap_create bpf_image)
	fi
	LIBS="$ac_save_LIBS"

        CFLAGS=$ac_CFLAGS_saved
        CPPFLAGS=$ac_CPPFLAGS_saved

	AC_SUBST(PCAP_LIBS)
        AC_SUBST(PCAP_INCLUDES)
])

