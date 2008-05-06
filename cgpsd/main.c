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

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <chemgps.h>

#include "cgpssqp.h"
#include "cgpsd.h"

struct options *opts;

void exit_handler(void)
{
	if(opts) {
		debug("cleaning up at exit...");
		
		if(opts->cgps) {
			if(opts->cgps->logfile) {
				free(opts->cgps->logfile);
				opts->cgps->logfile = NULL;
			}
			free(opts->cgps);
			opts->cgps = NULL;
		}
		if(opts->proj) {
			free(opts->proj);
			opts->proj = NULL;
		}
		if(opts->ipaddr) {
			if(opts->ipaddr != CGPSD_DEFAULT_ADDR) {
				free(opts->ipaddr);
			}
			opts->ipaddr = NULL;
		}
		if(opts->unaddr) {
			struct stat st;
			if(stat(opts->unaddr, &st) == 0) {
#ifdef HAVE_STAT_EMPTY_STRING_BUG
				if(strlen(opts->unaddr) != 0) {
#endif
				if(unlink(opts->unaddr) < 0) {
					logerr("failed unlink UNIX socket (%s)", opts->unaddr);
				}
#ifdef HAVE_STAT_EMPTY_STRING_BUG
				}
#endif
			}
			if(opts->unaddr != CGPSD_DEFAULT_SOCK) {
				free(opts->unaddr);
			}
			opts->unaddr = NULL;
		}
		if(opts->state & CGPSD_STATE_DAEMONIZED) {
			closelog();
		}		
		free(opts);
		opts = NULL;
	}
}

int main(int argc, char **argv)
{
	opts = malloc(sizeof(struct options));
	if(!opts) {
		die("failed alloc memory");
	}
	memset(opts, 0, sizeof(struct options));
	
	opts->cgps = malloc(sizeof(struct cgps_options));
	if(!opts->cgps) {
		die("failed alloc memory");
	}
	memset(opts->cgps, 0, sizeof(struct cgps_options));
	
	opts->prog = basename(argv[0]);
	opts->parent = getpid();
	
#ifdef HAVE_ATEXIT
	if(atexit(exit_handler) != 0) {
		logerr("failed register main exit handler");
	}
#endif
	
	parse_options(argc, argv, opts);
	
	if(init_socket(opts) < 0) {
		die("failed initilize socket");
	}
	if(!opts->interactive) {
		if(daemon(0, 0) < 0) {
			die("failed become daemon");
		}
		else {
			opts->state |= CGPSD_STATE_DAEMONIZED;
			openlog(opts->prog, LOG_PID, LOG_DAEMON);
		}
	}
	
	loginfo("daemon starting up (version: %s, project: %s)", PACKAGE_VERSION, basename(opts->proj));
	service(opts);
	loginfo("daemon shutting down...");

	close_socket(opts);
	
#if ! defined(HAVE_ATEXIT)
	debug("explicit calling exit handler");
	exit_handler();
#endif
	
	return 0;	
}
