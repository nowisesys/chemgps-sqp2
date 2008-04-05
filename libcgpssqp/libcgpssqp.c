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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
