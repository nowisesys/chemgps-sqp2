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
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include <getopt.h>

#include "cgpssqp.h"
#include "cgpsclt.h"

static void usage(const char *prog, const char *section)
{
	if(!section) {
		printf("%s - client for making prediction using libchemgps and Umetrics Simca-QP.\n", prog);
		printf("\n");
		printf("Usage: %s -f proj [options...]\n", prog);
		printf("Options:\n");
		printf("  -s, --sock[=path]:  Connect to UNIX socket [%s]\n", CGPSD_DEFAULT_SOCK);
		printf("  -H, --host=addr:    Connect to host (IP or hostname)\n");
		printf("  -p, --port=num:     Connect on port [%d]\n", CGPSD_DEFAULT_PORT);
		printf("  -i, --data=path:    Raw data input file (default=stdin)\n");
		printf("  -o, --output=path:  Write result to output file (default=stdout)\n");
		printf("  -r, --result=str:   Colon separated list of results to show (see -h result)\n");
		printf("  -f, --format=str:   Set ouput format (either plain or xml)\n");
		printf("  -4, --ipv4:         Only use IPv4\n");
		printf("  -6, --ipv6:         Only use IPv6\n");		
#if ! defined(NDEBUG)
		printf("  -d, --debug:        Enable debug output (allowed multiple times)\n");
#endif
		printf("  -v, --verbose:      Be more verbose in output\n");
		printf("  -h, --help:         This help\n");
		printf("  -V, --version:      Print version info to stdout\n");
		printf("\n");
		printf("This application is part of the ChemGPS project.\n");
		printf("Send bug reports to %s\n", PACKAGE_BUGREPORT);
	} else if(strcmp(section, "result") == 0) {
		const struct cgps_result_entry *entry;
		
		printf("The --result option arguments selects the prediction results to output.\n");
		printf("\n");
		printf("The following names can be used as argument for the --result (-r) option:\n");
		printf("\n");
		printf("%10s   %-20s %s\n", "value:", "name:", "description:");
		printf("%10s   %-20s %s\n", "------", "-----", "------------");
		for(entry = cgps_result_entry_list; entry->name; ++entry) {
			printf("%10d   %-20s %s\n", entry->value, entry->name, entry->desc);
		}
		printf("\n");
		printf("Example:\n");
		printf("  %s --result=csmwgrp:tps:serrlps\n", prog);
		printf("\n");
		printf("Values can be used instead of the names:\n");
		printf("  %s --result=4:6:12\n", prog);
	} else {	
		fprintf(stderr, "%s: no help avalible for '--help %s'\n", prog, section);
	}
}

static void version(const char *prog)
{
        printf("%s - part of package %s version %s\n", prog, PACKAGE_NAME, PACKAGE_VERSION);
	printf("\n");
	printf("A prediction client for cgpsd(8) using (at option) libchemgps and Umetrics Simca-QP\n");
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

void parse_options(int argc, char **argv, struct options *popt)
{
	static struct option options[] = {
                { "ipv4",    0, 0, '4' },
		{ "ipv6",    0, 0, '6' },		
		{ "sock",    2, 0, 's' },
		{ "host",    1, 0, 'H' },
		{ "port",    1, 0, 'p' },
                { "data",    1, 0, 'i' },
                { "output",  1, 0, 'o' },
		{ "result",  1, 0, 'r' },
		{ "format",  1, 0, 'f' }, 
#if ! defined(NDEBUG)
		{ "debug",   0, 0, 'd' },
#endif
		{ "verbose", 0, 0, 'v' },
		{ "help",    2, 0, 'h' },
		{ "version", 0, 0, 'V' }
	};
	int optindex, c;

	while((c = getopt_long(argc, argv, "46df:h::i:o:p:H:r:s:vV", options, &optindex)) != -1) {
		switch(c) {
		case '4':
			popt->family = AF_INET;
			break;
		case '6':
			popt->family = AF_INET6;
			break;
#if ! defined(NDEBUG)
		case 'd':
			popt->debug++;
			break;
#endif
		case 'f':
			if(strcmp("plain", optarg) == 0) {
				popt->cgps->format = CGPS_OUTPUT_FORMAT_PLAIN;
			} else if(strcmp("xml", optarg) == 0) {
				popt->cgps->format = CGPS_OUTPUT_FORMAT_XML;
			} else {
				die("unknown format '%s' argument for option -f", optarg);
			}
			break;
		case 'h':
			usage(popt->prog, optarg);
			exit(0);
		case 'H':
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
		case 'i':
			popt->data = malloc(strlen(optarg) + 1);
			if(!popt->data) {
				die("failed alloc memory");
			}
			strcpy(popt->data, optarg);
			break;
                case 'o':
			popt->output = malloc(strlen(optarg) + 1);
			if(!popt->output) {
				die("failed alloc memory");
			}
			strcpy(popt->output, optarg);
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
		case 'r':
			popt->cgps->result = cgps_get_predict_mask(optarg);
			break;
		case 's':
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
	if(popt->ipaddr && popt->unaddr) {
		die("both TCP and UNIX connection requested");
	}
	if(popt->ipaddr) {
		if(!popt->port) {
			popt->port = CGPSD_DEFAULT_PORT;
		}
		if(!popt->family) {
			popt->family = AF_UNSPEC;
		}
	}			  
	if(popt->unaddr) {
		struct stat st;
#ifdef HAVE_STAT_EMPTY_STRING_BUG
		if(strlen(popt->unaddr) == 0) {
			die("path of UNIX socket is empty");
		}
#endif
		if(stat(popt->unaddr, &st) < 0) {
			die("failed stat UNIX socket (%s)", popt->unaddr);
		}
		if(!S_ISSOCK(st.st_mode)) {
			die("file %s is not a UNIX socket", popt->unaddr);
		}
	}
	if(!popt->cgps->format) {
		popt->cgps->format = CGPS_OUTPUT_FORMAT_DEFAULT;
	}
		
	/*
	 * Dump options for debugging purpose.
	 */
	if(popt->debug) {
		debug("---------------------------------------------");
		debug("options:");
		if(popt->unaddr) {
			debug("  connect to unix socket = %s", popt->unaddr);
		}
		if(popt->ipaddr) {
			debug("  connect to %s on port %d", popt->ipaddr, popt->port);
		}
		if(popt->output) {
			debug("  saving result to %s", popt->output);
		}
		debug("  flags: debug = %s, verbose = %s", 
		      (popt->debug   ? "yes" : "no"), 
		      (popt->verbose ? "yes" : "no"));
		debug("---------------------------------------------");
	}
}
