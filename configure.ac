# -*- sh -*-  -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([chemgps-sqp2], 0.6.0, anders.lovgren@bmc.uu.se)
AM_INIT_AUTOMAKE([chemgps-sqp2], 0.6.0)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([chemgps], [cgps_project_load])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit memset strcspn strerror strspn])

AC_CONFIG_FILES([Makefile
                 docs/Makefile
		 libcgpssqp/Makefile
		 cgpsclt/Makefile
		 cgpsd/Makefile
		 cgpsstd/Makefile])
AC_OUTPUT
