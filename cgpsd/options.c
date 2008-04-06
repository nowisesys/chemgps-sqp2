/* Simca-QP application for the ChemGPS project.
 * Copyright (C) 2007 Anders Lövgren
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <libgen.h>
#include <chemgps.h>

#include "cgpssqp.h"

static void usage(const char *prog, const char *section)
{
	printf("%s - daemon for making prediction using Umetrics Simca-QP library.\n", prog);
	printf("\n");
	printf("Usage: %s -p proj -r data [options...]\n", prog);
	printf("Options:\n");
	printf("  -u, --proj=path:    Load project file\n");
	printf("  -s, --socket=path:    Listen on UNIX socket\n");
	printf("  -b, --bind=adddr:   Bind to interface (address)\n");
	printf("  -p, --port=num:     Listen on port\n");
	printf("  -l, --logfile=path: Use path as simca lib log\n");
	printf("  -d, --debug:        Enable debug output (allowed multiple times)\n");
	printf("  -v, --verbose:      Be more verbose in output\n");
	printf("  -h, --help:         This help\n");
	printf("  -V, --version:      Print version info to stdout\n");
	printf("\n");
	printf("This application is part of the ChemGPS project.\n");
	printf("Send bug reports to %s\n", PACKAGE_BUGREPORT);
}

static void version(const char *prog)
{
	printf("%s - %s %s\n", prog, PACKAGE_NAME, PACKAGE_VERSION);
	printf("\n");
	printf("Daemon for making prediction using Umetrics Simca-QP library.\n");
	printf("This application is part of the ChemGPS project.\n");
}

void parse_options(int argc, char **argv, struct options *opts)
{
	static struct option options[] = {
		{ "proj",    1, 0, 'u' },
		{ "socket",  1, 0, 's' },
		{ "bind",    1, 0, 'b' },
		{ "port",    1, 0, 'p' },		
		{ "logfile", 1, 0, 'l' },
		{ "debug",   0, 0, 'd' },
		{ "verbose", 0, 0, 'v' },
		{ "help",    2, 0, 'h' },
		{ "version", 0, 0, 'V' }
	};
	int index, c;
	struct stat st;

	if(argc < 3) {
		usage(opts->prog, NULL);
		exit(1);
	}
	
	while((c = getopt_long(argc, argv, "b:dhl:p:s:u:vV", options, &index)) != -1) {
		switch(c) {
		case 'b':
			opts->ipaddr = malloc(strlen(optarg) + 1);
			if(!opts->ipaddr) {
				die("failed alloc memory");
			}
			strcpy(opts->ipaddr, optarg);
			break;
		case 'd':
			opts->debug++;
			break;
		case 'h':
			usage(opts->prog, optarg);
			exit(0);
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
		case 's':
			opts->unaddr = malloc(strlen(optarg) + 1);
			if(!opts->unaddr) {
				die("failed alloc memory");
			}
			strcpy(opts->unaddr, optarg);
			break;
		case 'u':
			opts->proj = malloc(strlen(optarg) + 1);
			if(!opts->proj) {
				die("failed alloc memory");
			}
			strcpy(opts->proj, optarg);
			break;
		case 'v':
			opts->verbose++;
			break;
		case 'V':
			version(opts->prog);
			exit(0);
		}
	}

	/*
	 * Check arguments and set defaults.
	 */
	if(!opts->proj) {
		die("project file option (-p) is missing");
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
			debug("  bind to ipaddr %s on port %d", opts->ipaddr, opts->port);
		}
		debug("  flags: debug = %s, verbose = %s", 
		      (opts->debug   ? "yes" : "no"), 
		      (opts->verbose ? "yes" : "no"));
		debug("---------------------------------------------");
	}
}
