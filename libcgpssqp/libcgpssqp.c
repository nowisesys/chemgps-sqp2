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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef _GNU_SOURCE
# undef _GNU_SOURCE
#endif
#define _GNU_SOURCE 1

#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_LIBPTHREAD
# include <pthread.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <stdarg.h>
#include <chemgps.h>

#include "cgpssqp.h"

/*
 * This function parses the result option argument and set
 * the bitmask flags for each name.
 */
int cgps_get_predict_mask(const char *results)
{
	char *str, *curr, *next;
	int mask = 0;
	
	str = malloc(strlen(results) + 1);
	if(!str) {
		die("failed alloc memory");
	}
	next = strcpy(str, results);
	
	while((curr = strsep(&next, ":")) != NULL) {
		const struct cgps_result_entry *entry;
		int value  = atoi(curr);
		
		debug("reading result option %s (value=%d)", curr, value);
		if(value == 0) {
			entry = cgps_result_entry_value(curr);
		} else {
			entry = cgps_result_entry_name(value);
		}
		if(!entry) {
			free(str);
			die("failed lookup prediction result %s", curr);
		}
		debug("enable output of prediction result %s (%s)", entry->desc, entry->name);
		if(entry->value == PREDICTED_RESULTS_ALL) {
			cgps_result_setall(mask);
			break;
		}
		if(cgps_result_isset(value, entry->value)) {
			logwarn("output of prediction result %s (%s) is already set, possible typo?", 
				entry->desc, entry->name);
		}
		cgps_result_setopt(mask, entry->value);
	}
	
	free(str);
	return mask;
}

/*
 * Common logging function. Supports logging to stdout/stderr and syslog.
 */
void cgps_syslog(void *pref, int status, int code, int level, const char *file, unsigned int line, const char *fmt, ...)
{
	FILE *fs;
	char *buff;
	size_t size;

	struct options *opts = (struct options *)pref;

	/*
	 * Write log prefix:
	 */
	fs = open_memstream(&buff, &size);
	if(!(opts->syslog)) {
		if(opts->debug > 3) {
			fprintf(fs, "[0x%lu]", pthread_self());
		}
		if(opts->debug > 4) {
			struct timeval tv;
			if(gettimeofday(&tv, NULL) == 0) {
				fprintf(fs, "[%lu:%lu]", tv.tv_sec, tv.tv_usec);
			}
		}
		switch(level) {
		case LOG_ERR:
		case LOG_CRIT:
			fprintf(fs, "%s: error: ", opts->prog);
			break;
		case LOG_DEBUG:
			fprintf(fs, "debug: ");
			break;
		case LOG_WARNING:
			fprintf(fs, "%s: warning: ", opts->prog);
			break;
		}
	}
	
	/*
	 * Write variadic argument list:
	 */
	va_list ap;
	va_start(ap, fmt);
	vfprintf(fs, fmt, ap);
	va_end(ap);
	
	/*
	 * Write system call error string?
	 */
	if(code) {
		fprintf(fs, " (%s)", strerror(code));
	}
	if(level == LOG_DEBUG) {
		if(opts->debug > 1) {
			fprintf(fs, "\t[%s]", opts->prog);
		}
		if(opts->debug > 2) {
			fprintf(fs, " (%s:%d): ", file, line);
		}
	}

	/*
	 * Close write buffer and print buffer to destination log.
	 */
	fclose(fs);
	if(opts->syslog) {
		syslog(level, buff);
	}
	else {
		if(level == LOG_ERR || level == LOG_WARNING) {
			fprintf(stderr, "%s\n", buff);
		}
		else {
			printf("%s\n", buff);
		}
	}
		
	free(buff);
}

struct request_type
{
	const char *name;
	int value;
};

static const struct request_type request_types[] = {
	{ "predict", CGPSP_PROTO_PREDICT },
	{ "format", CGPSP_PROTO_FORMAT },
	{ "load", CGPSP_PROTO_LOAD },
	{ "data", CGPSP_PROTO_LOAD },
	{ "result", CGPSP_PROTO_RESULT },
	{ "target", CGPSP_PROTO_TARGET },
	{ "start", CGPSP_PROTO_START },
	{ "quit", CGPSP_PROTO_QUIT },
	{ "error", CGPSP_PROTO_ERROR },
	{ "count", CGPSP_PROTO_COUNT },
	{ "CGPSP \\d\\.\\d (\\w+: [a-z]+ ready)", CGPSP_PROTO_GREETING }, 
	{ NULL, CGPSP_PROTO_LAST }
};

/*
 * Read one line from socket stream to buffer.
 */
ssize_t read_request(char **buff, size_t *size, FILE *sock)
{
	ssize_t bytes;

	if(*buff) {
		memset(*buff, 0, *size);
	}
	if((bytes = getline(buff, size, sock)) != -1) {
		char *ptr = *buff + bytes - 1;
		*ptr = '\0';
	} else {
		logerr("failed read peer request");
	}
	return bytes;
}

#ifndef HAVE_STRCASECMP
/*
 * Simple replacement for missing strcasecmp()
 */
static int strcasecmp(const char *s1, const char *s2)
{
	while(*s1 && *s2) {
		if(toupper(*s1) != toupper(*s2)) break;
		++s1; ++s2;
	}
	return *s1 - *s2;
}
#endif

/*
 * Split request option.
 */
int split_request_option(char *buff, struct request_option *req)
{
	const struct request_type *type;
	char *p;
	
	if(buff) {
		if(opts->debug > 3) {
			debug("parsing request option: '%s'", buff);
		}
		req->option = buff;
		p = strchr(buff, ':');
		if(p) {
			*p = '\0';
			while(*++p) {
				if(*p != ' ')
					break;
			}
			req->value = *p ? p : NULL;
		}
		for(type = request_types; type->name; ++type) {
			if(opts->debug > 4) {
				debug("matching %s against %s", req->option, type->name);
			}
			if(strcasecmp(type->name, req->option) == 0) {
				if(opts->debug > 3) {
					if(req->value) {
						debug("matched option %s (value=%d) (option argument: %s)", 
						      type->name, type->value, req->value);
					} else {
						debug("matched option %s (value=%d) (no option argument)",
						      type->name, type->value);
					}
				}
				req->symbol = type->value;
				return req->symbol;
			}
		}
		if(strncasecmp(req->option, CGPSP_PROTO_NAME, strlen(CGPSP_PROTO_NAME)) == 0) {
			req->symbol = CGPSP_PROTO_GREETING;
			return CGPSP_PROTO_GREETING;
		}
	}
	return CGPSP_PROTO_LAST;
}
