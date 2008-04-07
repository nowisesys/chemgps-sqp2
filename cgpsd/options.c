/* Simca-QP application for the ChemGPS project.
 * Copyright (C) 2007 Anders L�vgren
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
 *  Contact: Anders L�vgren <anders.lovgren@bmc.uu.se>
 * ----------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <libgen.h>
#include <chemgps.h>

#include "cgpssqp.h"
#include "cgpsd.h"

static void usage(const char *prog, const char *section)
{
	printf("%s - daemon for making prediction using Umetrics Simca-QP library.\n", prog);
	printf("\n");
	printf("Usage: %s -f proj [options...]\n", prog);
	printf("Options:\n");
	printf("  -f, --proj=path:      Load project file\n");
	printf("  -u, --unix[=path]:    Listen on UNIX socket (socket path [%s])\n", CGPSD_DEFAULT_SOCK);
	printf("  -t, --tcp[=addr]:     Listen on TCP socket (interface address [%s])\n", CGPSD_DEFAULT_ADDR);
	printf("  -p, --port=num:       Listen on port [%d]\n", CGPSD_DEFAULT_PORT);
	printf("  -b, --backlog=num:    Listen queue length [%d]\n", CGPSD_QUEUE_LENGTH);
	printf("  -l, --logfile=path:   Use path as simca lib log\n");
	printf("  -i, --interactive:    Don't detach from controlling terminal\n");
	printf("  -d, --debug:          Enable debug output (allowed multiple times)\n");
	printf("  -v, --verbose:        Be more verbose in output\n");
	printf("  -h, --help:           This help\n");
	printf("  -V, --version:        Print version info to stdout\n");
	printf("\n");
	printf("This application is part of the ChemGPS project.\n");
	printf("Send bug reports to %s\n", PACKAGE_BUGREPORT);
}

static void version(const char *prog)
{
	printf("%s - package %s %s\n", prog, PACKAGE_NAME, PACKAGE_VERSION);
	printf("A daemon making prediction using Umetrics Simca-QP library.\n");
	printf("\n");
	printf(" * This program is distributed in the hope that it will be useful,\n");
	printf(" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf(" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n");
	printf(" * GNU General Public License for more details.\n");
	printf("\n");
	printf("The %s software is copyright (C) 2007-2008 by Anders L�vgren and\n", PACKAGE_NAME);
	printf("the Computing Department at Uppsala Biomedical Centre (BMC), Uppsala University.\n");
	printf("This application is part of the ChemGPS project.\n");
}

void parse_options(int argc, char **argv, struct options *opts)
{
	static struct option options[] = {
		{ "proj",    1, 0, 'f' },
		{ "unix",    2, 0, 'u' },
		{ "tcp",     2, 0, 't' },
		{ "port",    1, 0, 'p' },
		{ "backlog", 1, 0, 'b' },
		{ "logfile", 1, 0, 'l' },
		{ "interactive", 0, 0, 'i' },
		{ "debug",   0, 0, 'd' },
		{ "verbose", 0, 0, 'v' },
		{ "help",    0, 0, 'h' },
		{ "version", 0, 0, 'V' }
	};
	int index, c;
	struct stat st;

	while((c = getopt_long(argc, argv, "b:df:hil:p:t:u:vV", options, &index)) != -1) {
		switch(c) {
		case 'b':
			opts->backlog = atoi(optarg);
			break;
		case 'd':
			opts->debug++;
			break;
		case 'f':
			opts->proj = malloc(strlen(optarg) + 1);
			if(!opts->proj) {
				die("failed alloc memory");
			}
			strcpy(opts->proj, optarg);
			break;
		case 'h':
			usage(opts->prog, optarg);
			exit(0);
		case 'i':
			opts->interactive = 1;
			break;
		case 'l':
			opts->cgps->logfile = malloc(strlen(optarg) + 1);
			if(!opts->cgps->logfile) {
				die("failed alloc memory");
			}
			strcpy(opts->cgps->logfile, optarg);
			break;
		case 'p':
			opts->port = strtol(optarg, NULL, 10);
			if(!opts->port) {
				die("failed convert port number %s", optarg);
			}
			break;
		case 't':
			if(*optarg != '-') {
				opts->ipaddr = malloc(strlen(optarg) + 1);
				if(!opts->ipaddr) {
					die("failed alloc memory");
				}
				strcpy(opts->ipaddr, optarg);
			} else {
				--optind;
				opts->ipaddr = CGPSD_DEFAULT_ADDR;
			}
			break;
		case 'u':
			if(*optarg != '-') {
				opts->unaddr = malloc(strlen(optarg) + 1);
				if(!opts->unaddr) {
					die("failed alloc memory");
				}
				strcpy(opts->unaddr, optarg);
			} else {
				--optind;
				opts->unaddr = CGPSD_DEFAULT_SOCK;
			}
			break;
		case 'v':
			opts->verbose++;
			break;
		case 'V':
			version(opts->prog);
			exit(0);
		case '?':
			exit(1);
		}
	}

	/*
	 * Check arguments and set defaults.
	 */
	if(!opts->proj) {
		die("project file option (-f) is missing");
	}
	if(stat(opts->proj, &st) < 0) {
		die("failed stat project file (model) %s", opts->proj);
	}
	if(opts->cgps->logfile) {
		if(stat(opts->cgps->logfile, &st) < 0) {
			char *dir = dirname(opts->cgps->logfile);
			if(strlen(dir) == 1 && dir[0] == '.') {
				/*
				 * The logfile path is relative to current directory, i.e. "simca.log".
				 */
			} else {
				if(stat(dir, &st) < 0) {
					die("failed stat directory %s", dir);
				}
			}
		}
	}
	if(opts->ipaddr && !opts->port) {
		opts->port = CGPSD_DEFAULT_PORT;
	}			  
	if(!opts->backlog) {
		opts->backlog = CGPSD_QUEUE_LENGTH;
	}
	
	/*
	 * Dump options for debugging purpose.
	 */
	if(opts->debug) {
		debug("---------------------------------------------");
		debug("options:");
		debug("  project file path (model) = %s", opts->proj);
		if(opts->cgps->logfile) {
			debug("  simca lib logfile = %s", opts->cgps->logfile);
		}
		if(opts->unaddr) {
			debug("  using unix socket = %s", opts->unaddr);
		}
		if(opts->ipaddr) {
			debug("  bind to interface %s on port %d", opts->ipaddr, opts->port);
		}
		debug("  listen queue length = %d", opts->backlog);
		debug("  flags: debug = %s, verbose = %s, interactive = %s", 
		      (opts->debug   ? "yes" : "no"), 
		      (opts->verbose ? "yes" : "no"),
		      (opts->interactive ? "yes" : "no" ));
		debug("---------------------------------------------");
	}
}