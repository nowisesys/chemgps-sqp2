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
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#include <getopt.h>

#include "cgpssqp.h"

static void usage(const char *prog, const char *section)
{
	if(!section) {
		printf("%s - client for making prediction using Umetrics Simca-QP library.\n", prog);
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
		printf("  -d, --debug:        Enable debug output (allowed multiple times)\n");
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
	printf("%s - package %s %s\n", prog, PACKAGE_NAME, PACKAGE_VERSION);
	printf("The client for making prediction using Umetrics Simca-QP library.\n");
	printf("\n");
	printf(" * This program is distributed in the hope that it will be useful,\n");
	printf(" * but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	printf(" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n");
	printf(" * GNU General Public License for more details.\n");
	printf("\n");
	printf("The %s software is copyright (C) 2007-2008 by Anders Lövgren and\n", PACKAGE_NAME);
	printf("the Computing Department at Uppsala Biomedical Centre (BMC), Uppsala University.\n");
	printf("This application is part of the ChemGPS project.\n");
}

void parse_options(int argc, char **argv, struct options *opts)
{
	static struct option options[] = {
		{ "sock",    2, 0, 's' },
		{ "host",    1, 0, 'H' },
		{ "port",    1, 0, 'p' },
                { "data",    1, 0, 'i' },
                { "output",  1, 0, 'o' },
		{ "result",  1, 0, 'r' },
		{ "format",  1, 0, 'f' }, 
		{ "debug",   0, 0, 'd' },
		{ "verbose", 0, 0, 'v' },
		{ "help",    2, 0, 'h' },
		{ "version", 0, 0, 'V' }
	};
	int index, c;

	while((c = getopt_long(argc, argv, "df:h::i:o:p:H:r:s:vV", options, &index)) != -1) {
		switch(c) {
		case 'd':
			opts->debug++;
			break;
		case 'f':
			if(strcmp("plain", optarg) == 0) {
				opts->cgps->format = CGPS_OUTPUT_FORMAT_PLAIN;
			} else if(strcmp("xml", optarg) == 0) {
				opts->cgps->format = CGPS_OUTPUT_FORMAT_XML;
			} else {
				die("unknown format '%s' argument for option -f", optarg);
			}
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
		case 'i':
			opts->data = malloc(strlen(optarg) + 1);
			if(!opts->data) {
				die("failed alloc memory");
			}
			strcpy(opts->data, optarg);
			break;
                case 'o':
			opts->output = malloc(strlen(optarg) + 1);
			if(!opts->output) {
				die("failed alloc memory");
			}
			strcpy(opts->output, optarg);
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
		case 'r':
			opts->cgps->result = cgps_get_predict_mask(optarg);
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
	if(opts->ipaddr && opts->unaddr) {
		die("both TCP and UNIX connection requested");
	}
	if(opts->ipaddr && !opts->port) {
		opts->port = CGPSD_DEFAULT_PORT;
	}			  
	if(opts->unaddr) {
		struct stat st;
#ifdef HAVE_STAT_EMPTY_STRING_BUG
		if(strlen(opts->unaddr) == 0) {
			die("path of UNIX socket is empty");
		}
#endif
		if(stat(opts->unaddr, &st) < 0) {
			die("failed stat UNIX socket (%s)", opts->unaddr);
		}
	}
	if(!opts->cgps->format) {
		opts->cgps->format = CGPS_OUTPUT_FORMAT_DEFAULT;
	}
		
	/*
	 * Dump options for debugging purpose.
	 */
	if(opts->debug) {
		debug("---------------------------------------------");
		debug("options:");
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
