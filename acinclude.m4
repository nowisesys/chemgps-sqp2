## -*- sh -*-

dnl
dnl Disable output from debug() macros in code:
dnl
AC_DEFUN([CGPS_DISABLE_DEBUG],
[ 
  AC_ARG_ENABLE([debug], [  --disable-debug         Turn off debugging], 
  [ case "${enableval}" in
      yes) ndebug=true ;;
      no)  ndebug=false ;;
      *) AC_MSG_ERROR(bad value ${enableval} for --disable-debug) ;;
    esac 
  ], [ndebug=true])
  if test "x$ndebug" = "xfalse"; then
    AC_DEFINE(NDEBUG, 1, [Define to 1 to disable debug])
  fi
])

dnl
dnl Should the cgpsclt be linked without libchemgps so it can be
dnl installed on systems without libsimca?
dnl
AC_DEFUN([CGPS_ENABLE_FOREIGN_CLIENT],
[
  AC_ARG_ENABLE([foreign], [  --enable-foreign        Build client without dependencies of libchemgps],
  [ case "${enableval}" in
      yes) foreign=true ;;
      no)  foreign=false ;;
      *) AC_MSG_ERROR(bad value ${enableval} for --enable-foreign) ;;
    esac
  ], [foreign=false])
  AM_CONDITIONAL(FOREIGN_CLIENT, test "x$foreign" = "xtrue")
])

dnl
dnl Should the utilities be built?
dnl
AC_DEFUN([CGPS_ENABLE_UTILS],
[
  AC_ARG_ENABLE([utils], [  --enable-utils          Build utility applications from the utils directory],
  [ case "${enableval}" in
      yes) utils=true ;;
      no)  utils=false ;;
      *) AC_MSG_ERROR(bad value ${enableval} for --enable-utils) ;;
    esac
  ], [utils=false])
  AM_CONDITIONAL(BUILD_UTILS, test "x$utils" = "xtrue")
])


dnl
dnl Checking for libchemgps installation
dnl
AC_DEFUN([CGPS_CHECK_LIBCHEMGPS],
[
  AC_MSG_CHECKING([libchemgps])
  # Locations to check for libchemgps library and header:
  chemgps_lib_test=""
  chemgps_inc_test=""
  
  # Result of test:
  chemgps_lib_found=""
  chemgps_inc_found=""
  
  # Favor user preferences (--with-libchemgps):
  AC_ARG_WITH([libchemgps], [  --with-libchemgps=path        Set path to non-standard location of libchemgps],
  [ 
    chemgps_lib_test=${withval}
    chemgps_inc_test=${withval}
  ])

  # Append result from pkgconfig:
  pkgconfig="`which pkg-config`"
  if test "x$pkgconfig" != ""; then
    $pkgconfig --exists libchemgps
    if test "$?" == "0"; then
      chemgps_lib_test="${chemgps_lib_test} $($pkgconfig --variable=libdir libchemgps)"
      chemgps_inc_test="${chemgps_inc_test} $($pkgconfig --variable=includedir libchemgps)"
    fi
  fi
  
  # Append some standard locations:
  chemgps_lib_test="${chemgps_lib_test} /usr/local/lib /usr/local/chemgps /usr/lib/chemgps ../libchemgps/src/.libs ../../libchemgps/src/.libs"
  chemgps_inc_test="${chemgps_inc_test} /usr/local/include /usr/local/chemgps /usr/lib/chemgps ../libchemgps/src ../../libchemgps/src"
  
  for inc in ${chemgps_inc_test}; do
    if test -e "$inc/chemgps.h"; then
      chemgps_inc_found=$inc
      break
    fi
  done
  for lib in ${chemgps_lib_test}; do
    if test -e "$lib/libchemgps.la"; then
      chemgps_lib_found=$lib
      break
    fi
  done
  
  if test "x$chemgps_lib_found" != "x" -a "x$chemgps_inc_found" != "x"; then
    AC_MSG_RESULT([found, libs=${chemgps_lib_found}, include=${chemgps_inc_found}])
  else
    AC_MSG_ERROR([libchemgps not found])
  fi
  AC_SUBST(LIBCHEMGPS_LIBDIR, ${chemgps_lib_found})
  AC_SUBST(LIBCHEMGPS_INCDIR, ${chemgps_inc_found})  
])
