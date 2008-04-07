/* libcgpssqp - utility library for chemgps-sqp2
 * 
 * Copyright (C) 2007-2008 Computing Department at BMC,
 * Uppala Biomedical Centre, Uppsala University
 * ----------------------------------------------------------------------
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * ----------------------------------------------------------------------
 *  Contact: Anders Lövgren <anders.lovgren@bmc.uu.se>
 * ----------------------------------------------------------------------
 */

#ifndef __CGPSSQP_H__
#define __CGPSSQP_H__

#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <stdint.h>
#include <errno.h>
#include <chemgps.h>

#define CGPSD_DEFAULT_PORT 9401
#define CGPSD_DEFAULT_SOCK "/var/run/cgpsd.sock"
#define CGPSD_DEFAULT_ADDR "0.0.0.0"

struct options
{
	/*
	 * Should match order in cgps_options struct:
	 */
	const char *prog;     /* log prefix */
	int syslog;           /* use syslog */
	int debug;            /* enable debug */
	int verbose;          /* be more verbose */
	int batch;            /* enable batch job mode */
	/*
	 * Common members:
	 */
	const char *proj;     /* the project file (model) */	
	pid_t parent;         /* our prosess ID */	
	struct cgps_options *cgps;
	/*
	 * Client and server options:
	 */
	const char *data;     /* input data */
	const char *output;   /* output file */
	int numobs;           /* number of observations */
	int daemon;           /* running as daemon */
	int interactive;      /* don't detach from controlling terminal */
	char *unaddr;         /* unix socket */
	char *ipaddr;         /* ipv4/ipv6 addr */
	uint16_t port;        /* port number */
	int ipsock;           /* TCP socket */
	int unsock;           /* UNIX socket */
	int backlog;          /* listen queue length */
	int done;             /* time to exit? */
};

/*
 * Forward declare:
 */
extern struct options *opts;

/*
 * Log functions that logs to stdout/stderr (in debug mode) or syslog.
 *
 * These macros requires the GNUC compiler GCC or an ISO C99 standard
 * compliant compiler.
 */
#if defined(__GNUC__)
# define logerr(fmt, args...) do { \
	cgps_syslog(opts, 0 , errno ? errno : 0 , LOG_ERR , __FILE__ , __LINE__ , (fmt) , ## args); \
} while(0)

# define logwarn(fmt, args...) do { \
	cgps_syslog(opts, 0 , 0 , LOG_WARNING , __FILE__ , __LINE__ , (fmt) , ## args); \
} while(0)

# define loginfo(fmt, args...) do { \
	cgps_syslog(opts, 0 , 0 , LOG_INFO , __FILE__ , __LINE__ , (fmt) , ## args); \
} while(0)

# if ! defined(NDEBUG)
#  define debug(fmt, args...) do { \
	if(opts->debug) { \
		cgps_syslog(opts, 0 , 0 , LOG_DEBUG , __FILE__ , __LINE__ , (fmt) , ## args); \
	} \
} while(0)
# else /* ! defined(NDEBUG) */
#  define debug(fmt, args...)
# endif

# define die(fmt, args...) do { \
	cgps_syslog(opts, 1 , errno ? errno : 0 , LOG_CRIT , __FILE__ , __LINE__ , (fmt) , ## args); \
	if(!opts || opts->parent == getpid()) { \
		exit(1); \
	} else { \
		_exit(1); \
	} \
} while(0)
#else   /* ! defined(__GNUC__) */
# if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#  define logerr(...) do { \
	cgps_syslog(opts, 0 , errno ? errno : 0 , LOG_ERR , __FILE__ , __LINE__ , __VA_ARGS__); \
} while(0)

#  define logwarn(...) do { \
	cgps_syslog(opts, 0 , 0 , LOG_WARNING , __FILE__ , __LINE__ , __VA_ARGS__); \
} while(0)

#  define loginfo(...) do { \
	cgps_syslog(opts, 0 , 0 , LOG_INFO , __FILE__ , __LINE__ , __VA_ARGS__); \
} while(0)

#  if ! defined(NDEBUG)
#   define debug(...) do { \
	if(opts->debug) { \
		cgps_syslog(opts, 0 , 0 , LOG_DEBUG , __FILE__ , __LINE__ , __VA_ARGS__); \
	} \
} while(0)
#  else /* ! defined(NDEBUG) */
#   define debug(...)
#  endif

#  define die(...) do { \
	cgps_syslog(opts, 1 , errno ? errno : 0 , LOG_CRIT , __FILE__ , __LINE__ , __VA_ARGS__); \
	if(!opts || opts->parent == getpid()) { \
		exit(1); \
	} else { \
		_exit(1); \
	} \
} while(0)
# else  /* ! __STDC_VERSION__ >= 199901L */
/*
 * No support for variadic macros at all. Should we provide non-macro functions?
 */
#  define CGPS_SYSLOG_NOT_VARIADIC 1
#  error "Use the GNU C compiler (gcc) or enable C99 standard mode when compiling this code."
# endif
#endif  /* defined(__GNUC__) */

/*
 * Calculates the predict result mask from a colon separated string of 
 * prediction result names. The returned value can be used as result
 * set mask for libchemgps.
 */
int cgps_get_predict_mask(const char *results);

/*
 * Callbacks for libchemgps.
 */
void cgps_syslog(void *opts, int status, int code, int level, const char *file, unsigned int line, const char *fmt, ...);
int cgps_predict_data(struct cgps_project *proj, SQX_FloatMatrix *fmx, SQX_StringMatrix *smx, SQX_StringVector *names, int type);

#endif /* CGPSSQP_H__ */
