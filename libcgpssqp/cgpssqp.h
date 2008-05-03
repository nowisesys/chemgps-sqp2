/* Simca-QP predictions for the ChemGPS project.
 *
 * Copyright (C) 2007-2008 Anders Lövgren and the Computing Department,
 * Uppsala Biomedical Centre, Uppsala University.
 * 
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

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#include <errno.h>
#include <chemgps.h>

#define CGPSD_DEFAULT_PORT 9401
#define CGPSD_DEFAULT_SOCK "/var/run/cgpsd.sock"
#define CGPSD_DEFAULT_ADDR "0.0.0.0"

enum CGPS_APPLICATION { CGPS_APP_UNKNOWN, CGPS_STANDALONE, CGPS_DAEMON, CGPS_CLIENT, CGPS_DDOS };

#define CGPS_RESOLVE_RETRIES 5
#define CGPS_RESOLVE_TIMEOUT 2

#define CGPSP_PROTO_VERSION "1.0"
#define CGPSP_PROTO_CR      0x13
#define CGPSP_PROTO_LF      0x10
#define CGPSP_PROTO_NEWLINE htons(CGPSP_PROTO_CR << 8 | CGPSP_PROTO_LF)

#define CGPSP_PROTO_NAME "CGPSP"

/*
 * Values for request_type
 */
enum CGPSP_PROTO_VALUES {
	CGPSP_PROTO_GREETING = 0,/* client/server handshake */
	CGPSP_PROTO_PREDICT,     /* server set result */
	CGPSP_PROTO_FORMAT,      /* server set format */
	CGPSP_PROTO_LOAD,        /* client load data */
	CGPSP_PROTO_RESULT,      /* client read result */
	CGPSP_PROTO_TARGET,      /* set target host (cgpsddos only) */	
	CGPSP_PROTO_START,       /* start DDOS (cgpsddos only) */
	CGPSP_PROTO_QUIT,        /* quit (cgpsddos only) */
	CGPSP_PROTO_ERROR,       /* error message */
	CGPSP_PROTO_COUNT,       /* iterations (cgpsddos only) */	
	CGPSP_PROTO_LAST	
};

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
	int quiet;            /* be more quiet */
	int count;            /* repeate prediction count times */
	/*
	 * Common members:
	 */
	char *proj;           /* the project file (model) */	
	pid_t parent;         /* our prosess ID */	
	struct cgps_options *cgps;
	/*
	 * Client and server options:
	 */
	char *data;           /* input data */
	char *output;         /* output file */
	int numobs;           /* number of observations */
	int daemon;           /* running as daemon */
	int interactive;      /* don't detach from controlling terminal */
	char *unaddr;         /* unix socket */
	char *ipaddr;         /* ipv4/ipv6 addr */
	uint16_t port;        /* port number */
	int ipsock;           /* TCP socket */
	int unsock;           /* UNIX socket */
	int backlog;          /* listen queue length */
	int state;            /* daemon state */
	struct sigaction *newact; /* new signal action */
	struct sigaction *oldact; /* old signal action */	
};

/*
 * Peer connection endpoint.
 */
struct client
{
	const struct cgps_project *proj;
	struct options *opts;
	int sock;             /* client socket */
	int type;             /* application type */
	FILE *ss;             /* socket stream */
};

/*
 * A 'name: value' request option.
 */
struct request_option
{
	char *option;
	char *value;
	int symbol;
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
int cgps_predict_data(struct cgps_project *proj, void *data, SQX_FloatMatrix *fmx, SQX_StringMatrix *smx, SQX_StringVector *names, int type);

/*
 * Read one line from socket stream to buffer.
 */
ssize_t read_request(char **buff, size_t *size, FILE *sock);

/*
 * Split request option.
 */
int split_request_option(char *buff, struct request_option *req);

#endif /* CGPSSQP_H__ */
