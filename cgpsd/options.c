/* SIMCA-QP predictions for the ChemGPS project.
 *
 * Copyright (C) 2007-2018 Anders Lövgren and the Computing Department,
 * Uppsala Biomedical Centre, Uppsala University.
 * 
 * Copyright (C) 2018-2019 Anders Lövgren, Nowise Systems
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
 *  Contact: Anders Lövgren <andlov@nowise.se>
 * ----------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

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

static void usage(const char *prog)
{
	printf("%s - prediction daemon using libchemgps and Umetrics SIMCA-QP library.\n", prog);
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
#if ! defined(NDEBUG)
	printf("  -d, --debug:          Enable debug output (allowed multiple times)\n");
#endif
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
	printf("A multithreaded prediction daemon using libchemgps and Umetrics SIMCA-QP.\n");
	printf("This application is part of the ChemGPS project.\n");
	printf("\n");
	printf(" * This program is distributed in the hope that it will be useful,\n");
	printf(" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf(" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n");
	printf(" * GNU General Public License for more details.\n");
	printf("\n");
	printf("Copyright (C) 2007-2018 by Anders Lövgren and BMC-IT, Uppsala University.\n");
	printf("Copyright (C) 2018-2019 by Anders Lövgren, Nowise Systems.\n");
}

void parse_options(int argc, char **argv, struct options *popt)
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
#if ! defined(NDEBUG)
		{ "debug",   0, 0, 'd' },
#endif
		{ "verbose", 0, 0, 'v' },
		{ "quiet",   0, 0, 'q' },
		{ "help",    0, 0, 'h' },
		{ "version", 0, 0, 'V' }
	};
	int indexopt, c;
	struct stat st;

#ifdef HAVE_REALPATH
int path_max;
#endif
	
	while((c = getopt_long(argc, argv, "46b:df:hil:p:qt:u:vV", options, &indexopt)) != -1) {
		switch(c) {
                case '4':
			popt->family = AF_INET;
			break;
		case '6':
			popt->family = AF_INET6;
			break;
		case 'b':
			popt->backlog = atoi(optarg);
			break;
#if ! defined(NDEBUG)
		case 'd':
			popt->debug++;
			break;
#endif
		case 'f':
#ifdef HAVE_REALPATH
# if ! defined(HAVE_PATHCONF)
#  if defined(PATH_MAX)
			path_max = PATH_MAX;
#  else
#   error "No pathconf() and PATH_MAX is undefined"
#  endif
# else
			path_max = pathconf(optarg, _PC_PATH_MAX);
			if(path_max <= 0)
				path_max = 4096;
# endif	/* HAVE_PATHCONF */
			popt->proj = malloc(path_max + 1);
			if(!popt->proj) {
				die("failed alloc memory");
			}
			if(!realpath(optarg, popt->proj)) {
				die("failed get real path of project file %s", optarg);
			}
#else /* ! HAVE_REALPATH */
			if(*optarg != '/') {
				die("project path must be an absolute path");
			}
			popt->proj = malloc(strlen(optarg) + 1);
			if(!popt->proj) {
				die("failed alloc memory");
			}
			strcpy(popt->proj, optarg);
#endif
			break;
		case 'h':
			usage(popt->prog);
			exit(0);
		case 'i':
			popt->interactive = 1;
			break;
		case 'l':
			popt->cgps->logfile = malloc(strlen(optarg) + 1);
			if(!popt->cgps->logfile) {
				die("failed alloc memory");
			}
			strcpy(popt->cgps->logfile, optarg);
			break;
		case 'p':
#ifdef HAVE_STRTOL
			popt->port = strtol(optarg, NULL, 10);
#else
			popt->port = atoi(optarg);
#endif /* ! HAVE_STRTOL */
			if(!popt->port) {
				die("failed convert port number %s", optarg);
			}
			break;
		case 'q':
			popt->quiet = 1;
			break;
		case 't':
			if(*optarg != '-') {
				popt->ipaddr = malloc(strlen(optarg) + 1);
				if(!popt->ipaddr) {
					die("failed alloc memory");
				}
				strcpy(popt->ipaddr, optarg);
			} else {
				--optind;
				popt->ipaddr = (char *)CGPSD_DEFAULT_ADDR;
			}
			break;
		case 'u':
			if(*optarg != '-') {
				popt->unaddr = malloc(strlen(optarg) + 1);
				if(!popt->unaddr) {
					die("failed alloc memory");
				}
				strcpy(popt->unaddr, optarg);
			} else {
				--optind;
				popt->unaddr = (char *)CGPSD_DEFAULT_SOCK;
			}
			break;
		case 'v':
			popt->verbose++;
			break;
		case 'V':
			version(popt->prog);
			exit(0);
		case '?':
			exit(1);
		}
	}

	/*
	 * Check arguments and set defaults.
	 */
	if(!popt->ipaddr && !popt->unaddr) {
		die("neither -t or -u option used");
	}
	if(!popt->proj) {
		die("project file option (-f) is missing");
	}
	if(stat(popt->proj, &st) < 0) {
		die("failed stat project file (model) %s", popt->proj);
	}
	if(popt->cgps->logfile) {
		if(stat(popt->cgps->logfile, &st) < 0) {
			char *logfile;
			char *logdir;
			
			if(!(logfile = strdup(popt->cgps->logfile))) {
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
	if(popt->ipaddr) {
		if(!popt->port) {
			popt->port = CGPSD_DEFAULT_PORT;
		}
		if(!popt->family) {
			popt->family = AF_UNSPEC;
		}
	}			  
	if(!popt->backlog) {
		popt->backlog = CGPSD_QUEUE_LENGTH;
	}
	
	/*
	 * Dump options for debugging purpose.
	 */
	if(popt->debug) {
		debug("---------------------------------------------");
		debug("options:");
		debug("  project file path (model) = %s", popt->proj);
		if(popt->cgps->logfile) {
			debug("  simca lib logfile = %s", popt->cgps->logfile);
		}
		if(popt->unaddr) {
			debug("  bind UNIX socket %s", popt->unaddr);
		}
		if(popt->ipaddr) {
			const char *proto = "any protocol";
			if(popt->family != AF_UNSPEC) {
				proto = popt->family == AF_INET ? "ipv4" : "ipv6";
			}
			if(popt->ipaddr == CGPSD_DEFAULT_ADDR) {
				debug("  bind to TCP port %d on any interface (%s)", popt->port, proto);
			} else {
				debug("  bind to TCP port %d on interface %s (%s)", popt->port, popt->ipaddr, proto);
			}
		}
		debug("  listen queue length = %d", popt->backlog);
		debug("  flags: debug = %s, verbose = %s, interactive = %s", 
		      (popt->debug   ? "yes" : "no"), 
		      (popt->verbose ? "yes" : "no"),
		      (popt->interactive ? "yes" : "no" ));
		debug("---------------------------------------------");
	}
}
