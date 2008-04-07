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
#include <string.h>
#include <getopt.h>

#include "cgpssqp.h"

static void usage(const char *prog, const char *section)
{
	printf("%s - client for making prediction using Umetrics Simca-QP library.\n", prog);
	printf("\n");
	printf("Usage: %s -f proj [options...]\n", prog);
	printf("Options:\n");
	printf("  -s, --sock[=path]:    Connect to UNIX socket [%s]\n", CGPSD_DEFAULT_SOCK);
	printf("  -H, --host=addr:      Connect to host (IP or hostname)\n");
	printf("  -p, --port=num:       Connect on port [%d]\n", CGPSD_DEFAULT_PORT);
	/* printf("  -l, --logfile=path:   Use path as simca lib log\n"); */
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
	printf("The client for making prediction using Umetrics Simca-QP library.\n");
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
		{ "sock",    2, 0, 's' },
		{ "host",    1, 0, 'H' },
		{ "port",    1, 0, 'p' },
		/* { "logfile", 1, 0, 'l' }, */
		{ "debug",   0, 0, 'd' },
		{ "verbose", 0, 0, 'v' },
		{ "help",    0, 0, 'h' },
		{ "version", 0, 0, 'V' }
	};
	int index, c;

	while((c = getopt_long(argc, argv, "dhp:H:s:vV", options, &index)) != -1) {
		switch(c) {
		case 'd':
			opts->debug++;
			break;
		case 'h':
			usage(opts->prog, optarg);
			exit(0);
		case 'H':
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
		case 'p':
			opts->port = strtol(optarg, NULL, 10);
			if(!opts->port) {
				die("failed convert port number %s", optarg);
			}
			break;
		case 's':
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
	/* if(opts->cgps->logfile) { */
	/* 	if(stat(opts->cgps->logfile, &st) < 0) { */
	/* 		char *dir = dirname(opts->cgps->logfile); */
	/* 		if(strlen(dir) == 1 && dir[0] == '.') { */
	/* 		} else { */
	/* 			if(stat(dir, &st) < 0) { */
	/* 				die("failed stat directory %s", dir); */
	/* 			} */
	/* 		} */
	/* 	} */
	/* } */
	if(opts->ipaddr && opts->unaddr) {
		die("both TCP and UNIX connection requested");
	}
	if(opts->ipaddr && !opts->port) {
		opts->port = CGPSD_DEFAULT_PORT;
	}			  
	if(opts->unaddr) {
		struct stat st;
		if(stat(opts->unaddr, &st) < 0) {
			die("failed stat UNIX socket (%s)", opts->unaddr);
		}
	}
	
	/*
	 * Dump options for debugging purpose.
	 */
	if(opts->debug) {
		debug("---------------------------------------------");
		debug("options:");
		/* if(opts->cgps->logfile) { */
		/* 	debug("  simca lib logfile = %s", opts->cgps->logfile); */
		/* } */
		if(opts->unaddr) {
			debug("  connect to unix socket = %s", opts->unaddr);
		}
		if(opts->ipaddr) {
			debug("  connect to %s on port %d", opts->ipaddr, opts->port);
		}
		debug("  flags: debug = %s, verbose = %s", 
		      (opts->debug   ? "yes" : "no"), 
		      (opts->verbose ? "yes" : "no"));
		debug("---------------------------------------------");
	}
}
