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

#define _GNU_SOURCE
#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
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
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include <getopt.h>
#include <errno.h>
#include <libgen.h>
#include <chemgps.h>

#include "cgpssqp.h"
#include "cgpsd.h"

static void usage(const char *prog, const char *section)
{
	printf("%s - prediction daemon using libchemgps and Umetrics Simca-QP library.\n", prog);
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
	printf("  -4, --ipv4:           Only use IPv4\n");
	printf("  -6, --ipv6:           Only use IPv6\n");	
	printf("  -d, --debug:          Enable debug output (allowed multiple times)\n");
	printf("  -v, --verbose:        Be more verbose in output\n");
	printf("  -q, --quiet:          Suppress some output\n");
	printf("  -h, --help:           This help\n");
	printf("  -V, --version:        Print version info to stdout\n");
	printf("\n");
	printf("This application is part of the ChemGPS project.\n");
	printf("Send bug reports to %s\n", PACKAGE_BUGREPORT);
}

static void version(const char *prog)
{
	printf("%s - part of package %s version %s\n", prog, PACKAGE_NAME, PACKAGE_VERSION);
	printf("\n");
	printf("A multithreaded prediction daemon using libchemgps and Umetrics Simca-QP.\n");
	printf("This application is part of the ChemGPS project.\n");
	printf("\n");
	printf(" * This program is distributed in the hope that it will be useful,\n");
	printf(" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf(" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n");
	printf(" * GNU General Public License for more details.\n");
	printf("\n");
	printf("Copyright (C) 2007-2008 by Anders Lövgren and the Computing Department at\n");
	printf("Uppsala Biomedical Centre (BMC), Uppsala University.\n");
}

void parse_options(int argc, char **argv, struct options *opts)
{
	static struct option options[] = {
                { "ipv4",    0, 0, '4' },
		{ "ipv6",    0, 0, '6' },
		{ "proj",    1, 0, 'f' },
		{ "unix",    2, 0, 'u' },
		{ "tcp",     2, 0, 't' },
		{ "port",    1, 0, 'p' },
		{ "backlog", 1, 0, 'b' },
		{ "logfile", 1, 0, 'l' },
		{ "interactive", 0, 0, 'i' },
		{ "debug",   0, 0, 'd' },
		{ "verbose", 0, 0, 'v' },
		{ "quiet",   0, 0, 'q' },
		{ "help",    0, 0, 'h' },
		{ "version", 0, 0, 'V' }
	};
	int index, c;
	struct stat st;

	while((c = getopt_long(argc, argv, "46b:df:hil:p:qt:u:vV", options, &index)) != -1) {
		switch(c) {
                case '4':
			opts->family = AF_INET;
			break;
		case '6':
			opts->family = AF_INET6;
			break;
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
#ifdef HAVE_STRTOL
			opts->port = strtol(optarg, NULL, 10);
#else
			opts->port = atoi(optarg);
#endif /* ! HAVE_STRTOL */
			if(!opts->port) {
				die("failed convert port number %s", optarg);
			}
			break;
		case 'q':
			opts->quiet = 1;
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
	if(!opts->ipaddr && !opts->unaddr) {
		die("neither -t or -u option used");
	}
	if(!opts->proj) {
		die("project file option (-f) is missing");
	}
	if(stat(opts->proj, &st) < 0) {
		die("failed stat project file (model) %s", opts->proj);
	}
	if(opts->cgps->logfile) {
		if(stat(opts->cgps->logfile, &st) < 0) {
			char *logfile;
			char *logdir;
			
			if(!(logfile = strdup(opts->cgps->logfile))) {
				die("failed alloc memory");
			}
			logdir = dirname(logfile);
			if(strlen(logdir) == 1 && logdir[0] == '.') {
				/*
				 * The logfile path is relative to current directory, i.e. "simca.log".
				 */
			} else {
				if(stat(logdir, &st) < 0) {
					die("failed stat directory %s", logdir);
				}
			}
			free(logfile);
		}
	}
	if(opts->ipaddr) {
		if(!opts->port) {
			opts->port = CGPSD_DEFAULT_PORT;
		}
		if(!opts->family) {
			opts->family = AF_UNSPEC;
		}
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
			debug("  bind UNIX socket %s", opts->unaddr);
		}
		if(opts->ipaddr) {
			const char *proto = "any protocol";
			if(opts->family != AF_UNSPEC) {
				proto = opts->family == AF_INET ? "ipv4" : "ipv6";
			}
			if(opts->ipaddr == CGPSD_DEFAULT_ADDR) {
				debug("  bind to TCP port %d on any interface (%s)", opts->port, proto);
			} else {
				debug("  bind to TCP port %d on interface %s (%s)", opts->port, opts->ipaddr, proto);
			}
		}
		debug("  listen queue length = %d", opts->backlog);
		debug("  flags: debug = %s, verbose = %s, interactive = %s", 
		      (opts->debug   ? "yes" : "no"), 
		      (opts->verbose ? "yes" : "no"),
		      (opts->interactive ? "yes" : "no" ));
		debug("---------------------------------------------");
	}
}
