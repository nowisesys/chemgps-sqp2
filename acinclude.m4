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
