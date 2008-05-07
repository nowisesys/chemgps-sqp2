# -*- sh -*-  -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([chemgps-sqp2], 0.7.1, anders.lovgren@bmc.uu.se)
AM_INIT_AUTOMAKE([chemgps-sqp2], 0.7.1)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_CANONICAL_HOST

# Checks for programs.
CGPS_CHECK_COMPILER
AC_PROG_RANLIB

# Checks for libraries.
CGPS_CHECK_SIMCAQ
CGPS_CHECK_LIBCHEMGPS

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h linux/sockios.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Check threading support.
CGPS_CHECK_THREADING

# Checks for library functions. We need to add -fno-builtin or autoconf
# will not find functions builtin in GCC.
FLAGSC="$CFLAGS"
CFLAGS="-fno-builtin"
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit gettimeofday gethostbyname inet_ntoa memset select socket strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strspn strtol strtoul])
CFLAGS="$FLAGSC"

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
