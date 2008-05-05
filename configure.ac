# -*- sh -*-  -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([chemgps-sqp2], 0.7.1, anders.lovgren@bmc.uu.se)
AM_INIT_AUTOMAKE([chemgps-sqp2], 0.7.1)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
## AC_CHECK_LIB([chemgps], [cgps_project_load])
AC_CHECK_LIB([pthread], [pthread_create])
if test "x${ac_cv_lib_pthread_pthread_create}" == "xyes"; then
  CFLAGS="$CFLAGS -pthread"
  AC_CHECK_LIB([pthread], [pthread_yield], [
    AC_DEFINE([HAVE_PTHREAD_YIELD], [1], [Define to 1 if libpthread has function pthread_yield])
  ])
  AC_MSG_CHECKING([if pthread_yield needs __USE_GNU defined])
  AC_COMPILE_IFELSE([
    #include <ptread.h>
    main(void) { pthread_yield(); }
    ], [AC_MSG_RESULT(no)], [
      AC_MSG_RESULT(yes)
      CPPFLAGS="$CPPFLAGS -D__USE_GNU"
    ]
  )
fi

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h linux/sockios.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit gettimeofday gethostbyname inet_ntoa memset select socket strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strspn strtol strtoul])

CGPS_CHECK_SIMCAQ
CGPS_CHECK_LIBCHEMGPS
CGPS_DISABLE_DEBUG
CGPS_ENABLE_UTILS
CGPS_ENABLE_FOREIGN_CLIENT

AC_CONFIG_FILES([Makefile
		 libcgpssqp/Makefile
		 cgpsclt/Makefile
		 cgpsd/Makefile
		 cgpsstd/Makefile
                 docs/Makefile
		 utils/Makefile
		 utils/cgpsddos/Makefile])
AC_OUTPUT
