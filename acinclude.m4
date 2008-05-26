## -*- sh -*-
##
## Autoconf extensions for project ChemGPS.
##
## Author: Anders Lövgren <lespaul@algonet.se>
## Date:   2008-05-07

dnl
dnl This function is borrowed from KDE (The K Desktop Environment) and modified
dnl to work with CC instead of CXX compiler.
dnl
AC_DEFUN([CGPS_CHECK_COMPILER_FLAG],
[
  AC_MSG_CHECKING([whether $CC supports -$1])
  cgps_cache=`echo $1 | sed 'y% .=/+-,%____p__%'`
  AC_CACHE_VAL(cgps_cv_prog_c_${cgps_cache},
  [
    AC_LANG_SAVE
    AC_LANG_C
    save_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -$1"
    AC_TRY_LINK([],[ return 0; ], [eval "cgps_cv_prog_c_${cgps_cache}=yes"], [])
    CFLAGS="$save_CFLAGS"
    AC_LANG_RESTORE
  ])
  if eval "test \"`echo '$cgps_cv_prog_c_'${cgps_cache}`\" = yes"; then
    AC_MSG_RESULT(yes)
         : $2
  else
    AC_MSG_RESULT(no)
         : $3
  fi
])

dnl
dnl Check compiler flags for debugging, warnings and profiling should be enabled.
dnl Some C++ checks are included even though its currently not used in this project.
dnl
AC_DEFUN([CGPS_CHECK_COMPILER],
[ 
    AC_ARG_ENABLE(debug,
    AC_HELP_STRING([--enable-debug=arg], [Enable debug options (no|std|yes|full) [default=std]]),
    [
      case $enableval in
	full)
	  cgps_use_debug_code="full"
	  cgps_use_debug_define="yes"
	  ;;
        yes)
	  cgps_use_debug_code="yes"
	  cgps_use_debug_define="yes"
	  ;;
	no)
	  cgps_use_debug_code="no"
	  cgps_use_debug_define="no"
	  ;;
	std|*)
	  cgps_use_debug_code="no"
	  cgps_use_debug_define="yes"
	  ;;
      esac
    ],
    [
      cgps_use_debug_code="no"
      cgps_use_debug_define="yes"
    ])
    
    AC_ARG_ENABLE(warnings,
    AC_HELP_STRING([--disable-warnings], [Disables compilation with -Wall and similar]),
    [
      if test $enableval = "no"; then
        cgps_use_warnings="no"
      else
        cgps_use_warnings="yes"
      fi
    ], [cgps_use_warnings="yes"])
    
    AC_ARG_ENABLE(profile,
    AC_HELP_STRING([--enable-profile], [Include profiling info in binaries [default=no]]),
    [cgps_use_profiling=$enableval],
    [cgps_use_profiling="no"]
    )

    dnl this prevents stupid AC_PROG_CC to add "-g" to the default CFLAGS
    FLAGSC=" $CFLAGS"
    CFLAGS=""
    AC_PROG_CC
    AC_PROG_CPP

    if test "$GCC" = "yes" || test "$CC" = "KCC"; then
      if test "$cgps_use_debug_code" != "no"; then
        if test "$CC" = "KCC"; then
	  CFLAGS="+K0 -Wall -pedantic -W -Wpointer-arith -Wwrite-strings $CFLAGS"
	else
	  if test "$cgps_use_debug_code" = "full"; then
	    CFLAGS="-g3 -fno-inline $CFLAGS"
	  else
	    CFLAGS="-g -O -fno-reorder-blocks -fno-schedule-insns -fno-inline $CFLAGS"
	  fi
	fi
	CGPS_CHECK_COMPILER_FLAG(fno-builtin, [CFLAGS="-fno-builtin $CFLAGS"])
      else
	if test "$CC" = "KCC"; then
	  CFLAGS="+K3 $CFLAGS"
	else
	  CFLAGS="-O2 $CFLAGS"
	fi
      fi
    fi
    
    if test "$cgps_use_debug_define" == "no"; then
      CFLAGS="-DNDEBUG $CFLAGS"
    fi
	    
    if test "$cgps_use_profiling" = "yes"; then
      CGPS_CHECK_COMPILER_FLAG(pg,
      [
        CFLAGS="-pg $CFLAGS"
      ])
    fi

    if test "$cgps_use_warnings" = "yes"; then
      if test "$GCC" = "yes"; then
        CFLAGS="-Wall -W -Wpointer-arith $CFLAGS"
	if test "$cgps_use_debug_code" != "no"; then
	  CFLAGS="-Werror $CFLAGS"
	fi
	case $host in
	  *-*-linux-gnu)
	    CFLAGS="-std=iso9899:1990 -W -Wall -Wchar-subscripts -Wshadow -Wpointer-arith -Wmissing-prototypes -Wwrite-strings -D_XOPEN_SOURCE=600 -D_BSD_SOURCE $CFLAGS"
	    CXXFLAGS="-ansi -D_XOPEN_SOURCE=600 -D_BSD_SOURCE -Wcast-align -Wconversion -Wchar-subscripts $CXXFLAGS"
	    CGPS_CHECK_COMPILER_FLAG(Wmissing-format-attribute, [CFLAGS="$CFLAGS -Wformat-security -Wmissing-format-attribute"])
	    ;;
	esac
	CGPS_CHECK_COMPILER_FLAG(Wundef,[CFLAGS="-Wundef $CFLAGS"])
	CGPS_CHECK_COMPILER_FLAG(Wno-long-long,[CFLAGS="-Wno-long-long $CFLAGS"])
	CGPS_CHECK_COMPILER_FLAG(Wno-non-virtual-dtor,[CXXFLAGS="$CXXFLAGS -Wno-non-virtual-dtor"])
      fi
    fi
    
    if test "$GXX" = "yes"; then
      CGPS_CHECK_COMPILER_FLAG(fno-check-new, [CXXFLAGS="$CXXFLAGS -fno-check-new"])
      CGPS_CHECK_COMPILER_FLAG(fno-common, [CXXFLAGS="$CXXFLAGS -fno-common"])
      CGPS_CHECK_COMPILER_FLAG(fexceptions, [CXXFLAGS="$CXXFLAGS -fexceptions"])
      ENABLE_PERMISSIVE_FLAG="-fpermissive"
    fi
    if test "$GCC" = "yes"; then
      CGPS_CHECK_COMPILER_FLAG(fno-common, [CFLAGS="$CFLAGS -fno-common"])
      CGPS_CHECK_COMPILER_FLAG(fexceptions, [CFLAGS="$CFLAGS -fexceptions"])
    fi
		    
    case "$host" in
      *-*-irix*)  
        if test "$GXX" = yes; then 
	  CXXFLAGS="-D_LANGUAGE_C_PLUS_PLUS -D__LANGUAGE_C_PLUS_PLUS $CXXFLAGS"
	fi
	;;
      *-*-sysv4.2uw*) 
        CXXFLAGS="-D_UNIXWARE $CXXFLAGS"
        CFLAGS="-D_UNIXWARE $CFLAGS"
	;;
      *-*-sysv5uw7*) 
        CXXFLAGS="-D_UNIXWARE7 $CXXFLAGS"
        CFLAGS="-D_UNIXWARE7 $CFLAGS"
	;;
      *-*-solaris*)
        if test "$GXX" = yes; then
	  libstdcpp=`$CXX -print-file-name=libstdc++.so`
          if test ! -f $libstdcpp; then
	    AC_MSG_ERROR([You\'ve compiled gcc without --enable-shared. Please recompile gcc with --enable-shared to receive a libstdc++.so])
	  fi
        fi
	;;
    esac
    
    dnl Merge with flags given on command line.
    CFLAGS="$CFLAGS $FLAGSC"
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
  AC_MSG_CHECKING([for libchemgps])
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

  chemgps_stdinc=true
  chemgps_develop=false

  # Test some standard locations:
  chemgps_lib_test="${chemgps_lib_test} /usr/lib /usr/local/lib /usr/local/chemgps /usr/local/libchemgps \
                    /usr/lib/chemgps /usr/lib/libchemgps"
  chemgps_inc_test="${chemgps_inc_test} /usr/include /usr/local/include /usr/local/chemgps /usr/local/libchemgps \
                    /usr/lib/chemgps /usr/lib/libchemgps"
  
  for inc in ${chemgps_inc_test}; do
    if test -e "$inc/chemgps.h"; then
      chemgps_inc_found=$inc
      break
    fi
    if test -e "$inc/include/chemgps.h"; then
      chemgps_inc_found=$inc/include
      break
    fi
  done
  for lib in ${chemgps_lib_test}; do
    if test -e "$lib/libchemgps.la"; then
      chemgps_lib_found=$lib
      break
    fi
    if test -e "$lib/lib/libchemgps.la"; then
      chemgps_lib_found=$lib/lib
      break
    fi
  done

  # Test for development versions (if libchemgps is not yet found):
  if test "x$chemgps_lib_found" == "x" -o "x$chemgps_inc_found" == "x"; then
    chemgps_lib_test="../libchemgps/src/.libs ../../libchemgps/src/.libs"
    chemgps_inc_test="../libchemgps/src ../../libchemgps/src"

    if test "x$chemgps_inc_found" == "x"; then
      for inc in ${chemgps_inc_test}; do
        if test -e "$inc/chemgps.h"; then
          chemgps_inc_found=$inc
          chemgps_develop=true
          break
        fi
      done
    fi
    if test "x$chemgps_lib_found" == "x"; then
      for lib in ${chemgps_lib_test}; do
        if test -e "$lib/libchemgps.la"; then
          chemgps_lib_found=$lib
          chemgps_develop=true
          break
        fi
      done
    fi
  fi
  
  if test "x$chemgps_lib_found" != "x" -a "x$chemgps_inc_found" != "x"; then
    if test "${chemgps_inc_found}" != "/usr/include" -a \
            "${chemgps_inc_found}" != "/usr/local/include"; then
      chemgps_stdinc=false
    fi
    if test "x${chemgps_stdinc}" == "xfalse"; then
      if test "x${chemgps_develop}" == "xtrue"; then
        CFLAGS="$CFLAGS -I../${chemgps_inc_found} -I../../${chemgps_inc_found}"
        LDFLAGS="$LDFLAGS -L../${chemgps_lib_found} -L../../${chemgps_lib_found}"
      else 
        CFLAGS="$CFLAGS -I${chemgps_inc_found}"
        LDFLAGS="$LDFLAGS -L${chemgps_lib_found}"
      fi
    fi      
    AC_MSG_RESULT([found, libs=${chemgps_lib_found}, include=${chemgps_inc_found}])
  else
    AC_MSG_ERROR([libchemgps not found])
  fi
])

dnl
dnl Checking for Simca-Q installation
dnl
AC_DEFUN([CGPS_CHECK_SIMCAQ],
[
  AC_MSG_CHECKING([for Simca Q])
  # Locations to check for libsimcaq library and header:
  simcaq_lib_test=""
  simcaq_inc_test=""
  
  # Result of test:
  simcaq_lib_found=""
  simcaq_inc_found=""
  
  # Favor user preferences (--with-simcaq):
  AC_ARG_WITH([simcaq], [  --with-simcaq=path        Set path to non-standard location of libsimcaq],
  [ 
    simcaq_lib_test=${withval}
    simcaq_inc_test=${withval}
  ])

  # Append result from pkgconfig:
  pkgconfig="`which pkg-config`"
  if test "x$pkgconfig" != ""; then
    $pkgconfig --exists libsimcaq
    if test "$?" == "0"; then
      simcaq_lib_test="${simcaq_lib_test} $($pkgconfig --variable=libdir libsimcaq)"
      simcaq_inc_test="${simcaq_inc_test} $($pkgconfig --variable=includedir libsimcaq)"
    fi
    $pkgconfig --exists simcaq
    if test "$?" == "0"; then
      simcaq_lib_test="${simcaq_lib_test} $($pkgconfig --variable=libdir simcaq)"
      simcaq_inc_test="${simcaq_inc_test} $($pkgconfig --variable=includedir simcaq)"
    fi
  fi
  
  # Append some standard locations:
  simcaq_lib_test="${simcaq_lib_test} /usr/lib /usr/local/lib \
                   /usr/lib/simcaq /usr/lib/libsimcaq \
                   /usr/lib/simcaq/lib /usr/lib/libsimcaq/lib \
                   /usr/local/simcaq /usr/local/libsimcaq \
                   /usr/local/simcaq/lib /usr/local/libsimcaq/lib"
  simcaq_inc_test="${simcaq_inc_test} /usr/include /usr/local/include \
		   /usr/lib/simcaq /usr/lib/libsimcaq \
		   /usr/lib/simcaq/include /usr/lib/libsimcaq/include \
                   /usr/local/simcaq /usr/local/libsimcaq \		   
                   /usr/local/simcaq/include /usr/local/libsimcaq/include"
  
  for inc in ${simcaq_inc_test}; do
    if test -e "$inc/SQXCInterface.h"; then
      simcaq_inc_found=$inc
      break
    fi
  done
  for lib in ${simcaq_lib_test}; do
    if test -e "$lib/libsimcaq.so"; then
      simcaq_lib_found=$lib
      break
    fi
  done
  
  if test "x$simcaq_lib_found" != "x" -a "x$simcaq_inc_found" != "x"; then
    AC_MSG_RESULT([found, libs=${simcaq_lib_found}, include=${simcaq_inc_found}])
  else
    AC_MSG_ERROR([libsimcaq not found])
  fi
  AC_SUBST(SIMCAQ_LIBDIR, ${simcaq_lib_found})
  AC_SUBST(SIMCAQ_INCDIR, ${simcaq_inc_found})  
])

dnl 
dnl Detect pthread library functions and process scheduling.
dnl 
AC_DEFUN([CGPS_CHECK_THREADING],
[
  AC_CHECK_LIB([pthread], [pthread_create])
  AC_CHECK_FUNCS([pthread_yield sched_yield])
  AC_CHECK_HEADERS([sched.h])
  if test "x${ac_cv_lib_pthread_pthread_create}" == "xyes"; then
    CFLAGS="$CFLAGS -pthread"
    CPPFLAGS="$CPPFLAGS -D_REENTRANT -D_THREAD_SAFE"
    if test "$CC" == "gcc"; then
      AC_CHECK_LIB([gcc_s], [main])
      # LDFLAGS="$LDFLAGS -lgcc_s"
    fi
    AC_CHECK_LIB([pthread], [pthread_yield], 
    [
      AC_DEFINE([HAVE_PTHREAD_YIELD], [1], [Define to 1 if libpthread has function pthread_yield])
    ])
    AC_MSG_CHECKING([if pthread_yield needs _GNU_SOURCE defined])
    AC_COMPILE_IFELSE(
    [
      #include <pthread.h>
      int main(void) { pthread_yield(); return 0; }
      ], [AC_MSG_RESULT(no)], [
      AC_MSG_RESULT(yes)
      CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE"
    ])
    AC_MSG_CHECKING([if pthread_yield needs to be declared])
    AC_COMPILE_IFELSE([
      #ifdef _GNU_SOURCE
      # undef _GNU_SOURCE
      #endif
      #define _GNU_SOURCE 1
      #include <features.h>
      #include <pthread.h>
      int main(void) { pthread_yield(); return 0; }
      ], [AC_MSG_RESULT(no)], 
      [
        AC_MSG_RESULT(yes)
        AC_DEFINE([NEED_PTHREAD_YIELD_DECL], [1], [Define to 1 if pthread_yield() needs to be declared])
      ])
  fi
])
