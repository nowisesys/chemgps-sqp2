# -*- sh -*-  -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([chemgps-sqp2], 0.6.1, anders.lovgren@bmc.uu.se)
AM_INIT_AUTOMAKE([chemgps-sqp2], 0.6.1)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
## AC_CHECK_LIB([chemgps], [cgps_project_load])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit gethostbyname inet_ntoa memset select socket strcasecmp strchr strcspn strerror strspn strtol])

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
		 utils/stress/Makefile])
AC_OUTPUT
