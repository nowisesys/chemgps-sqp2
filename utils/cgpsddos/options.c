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
#include <getopt.h>

#include "cgpsddos.h"
#include "cgpssqp.h"

static void usage(const char *prog, const char *section)
{
	if(!section) {
		printf("%s - distributed DOS (denial of service) application.\n", prog);
		printf("This application should only be used for testing the cgpsd daemon.\n");
		printf("\n");
		printf("Usage: master: %s -m -c file -t target -p port [options...]\n", prog);
		printf("       slave:  %s -s -a master [options...]\n", prog);
		printf("       local:  %s -u sock [options...]\n", prog);
		printf("\n");
		printf("Master options:\n");
		printf("  -m, --master:       Run as DDOS master\n");
		printf("  -c, --slaves=file:  File containing list of slaves (default=stdin)\n");
		printf("  -t, --target=addr[:port]:  DDOS targets IP or hostname including port\n");
		printf("  -p, --port=num:     Listen on port [%d]\n", CGPSDDOS_MASTER_PORT);
		printf("  -i, --data=path:    Raw data input file\n");
		printf("  -r, --result=str:   Colon separated list of results to show (see -h result)\n");
		printf("  -f, --format=str:   Set ouput format (either plain or xml)\n");
		printf("  -w, --timeout=num:  Timeout waiting for peer response [%d]\n", CGPSDDOS_PEER_TIMEOUT);
		printf("  -n, --count=num:    Repeate prediction num times\n");
		printf("\n");
		printf("Slave options:\n");
		printf("  -s, --slave:        Run as DDOS slave\n");
		printf("  -a, --accept=host:  Only accept connections from this master host\n");
		printf("  -p, --port=num:     Listen on port [%d]\n", CGPSDDOS_SLAVE_PORT);
		printf("\n");
		printf("Local options:\n");
		printf("  -u, --sock[=path]:  Connect to UNIX socket [%s]\n", CGPSD_DEFAULT_SOCK);
		printf("  -i, --data=path:    Raw data input file (default=stdin)\n");
		printf("  -r, --result=str:   Colon separated list of results to show (see -h result)\n");
		printf("  -f, --format=str:   Set ouput format (either plain or xml)\n");
		printf("\n");
		printf("Common options:\n");
		printf("  -4, --ipv4:         Only use IPv4\n");
		printf("  -6, --ipv6:         Only use IPv6\n");
#if ! defined(NDEBUG)
		printf("  -d, --debug:        Enable debug output (allowed multiple times)\n");
#endif
		printf("  -v, --verbose:      Be more verbose in output\n");
		printf("  -q, --quiet:        Suppress some output\n");
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
	printf("The distributed DOS (denial of service) application.\n");
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

void parse_options(int argc, char **argv, struct cgpsddos *ddos)
{
	static struct option options[] = {
		{ "ipv4",    0, 0, '4' },
		{ "ipv6",    0, 0, '6' },
		{ "master",  0, 0, 'm' },
		{ "slaves",  1, 0, 'c' },
		{ "target",  1, 0, 't' },
		{ "port",    1, 0, 'p' },
                { "data",    1, 0, 'i' },
		{ "result",  1, 0, 'r' },
		{ "format",  1, 0, 'f' }, 
		{ "slave",   0, 0, 's' },
		{ "accept",  0, 0, 'a' },		
		{ "sock",    2, 0, 'u' },
		{ "timeout", 1, 0, 'w' },
		{ "count",   1, 0, 'n' },
#if ! defined(NDEBUG)
		{ "debug",   0, 0, 'd' },
#endif
		{ "verbose", 0, 0, 'v' },
		{ "quiet",   0, 0, 'q' },
		{ "help",    2, 0, 'h' },
		{ "version", 0, 0, 'V' }
	};
	int optindex, c;

	while((c = getopt_long(argc, argv, "46a:c:df:h::t:i:mn:p:qr:su:w:vV", options, &optindex)) != -1) {
		switch(c) {
		case '4':
			ddos->family = AF_INET;
			break;
		case '6':
			ddos->family = AF_INET6;
			break;
		case 'a':
			ddos->accept = malloc(strlen(optarg) + 1);
			if(!ddos->accept) {
				die("failed alloc memory");
			}
			strcpy(ddos->accept, optarg);
			break;
		case 'c':
			ddos->slaves = malloc(strlen(optarg) + 1);
			if(!ddos->slaves) {
				die("failed alloc memory");
			}
			strcpy(ddos->slaves, optarg);
			break;
#if ! defined(NDEBUG)
		case 'd':
			ddos->opts->debug++;
			break;
#endif
		case 'f':
			if(strcmp("plain", optarg) == 0) {
				ddos->opts->cgps->format = CGPS_OUTPUT_FORMAT_PLAIN;
			} else if(strcmp("xml", optarg) == 0) {
				ddos->opts->cgps->format = CGPS_OUTPUT_FORMAT_XML;
			} else {
				die("unknown format '%s' argument for option -f", optarg);
			}
			break;
		case 'h':
			usage(ddos->opts->prog, optarg);
			exit(0);
		case 'i':
			ddos->opts->data = malloc(strlen(optarg) + 1);
			if(!ddos->opts->data) {
				die("failed alloc memory");
			}
			strcpy(ddos->opts->data, optarg);
			break;
		case 'm':
			ddos->mode = CGPSDDOS_MASTER;
			break;
		case 'n':
			ddos->opts->count = strtoul(optarg, NULL, 10);
			break;
		case 'p':
#ifdef HAVE_STRTOL
			ddos->opts->port = strtol(optarg, NULL, 10);
#else
			ddos->opts->port = atoi(optarg);
#endif /* ! HAVE_STRTOL */
			if(!ddos->opts->port) {
				die("failed convert port number %s", optarg);
			}
			break;
		case 'q':
			ddos->opts->quiet = 1;
			break;
		case 'r':
			ddos->opts->cgps->result = cgps_get_predict_mask(optarg);
			break;
		case 's':
			ddos->mode = CGPSDDOS_SLAVE;
			break;
		case 't':
			ddos->opts->ipaddr = malloc(strlen(optarg) + 1);
			if(!ddos->opts->ipaddr) {
				die("failed alloc memory");
			}
			strcpy(ddos->opts->ipaddr, optarg);
			break;
		case 'u':
			if(*optarg != '-') {
				ddos->opts->unaddr = malloc(strlen(optarg) + 1);
				if(!ddos->opts->unaddr) {
					die("failed alloc memory");
				}
				strcpy(ddos->opts->unaddr, optarg);
			} else {
				--optind;
				ddos->opts->unaddr = (char *)CGPSD_DEFAULT_SOCK;
			}
			break;
		case 'v':
			ddos->opts->verbose++;
			break;
		case 'V':
			version(ddos->opts->prog);
			exit(0);
		case 'w':
			ddos->timeout = strtoul(optarg, NULL, 10);
			break;
		case '?':
			exit(1);
		}
	}

	/*
	 * Check arguments and set defaults.
	 */
	if(ddos->accept && ddos->mode != CGPSDDOS_SLAVE) {
		die("the accept connections option (-a) is only valid in slave mode");
	}
	if(ddos->slaves && ddos->mode != CGPSDDOS_MASTER) {
		die("the slaves file option (-c) is only valid in master mode");
	}
	if(ddos->opts->cgps->format && ddos->mode == CGPSDDOS_SLAVE) {
		die("the format option (-f) is only valid in master or local mode");
	}
	if(ddos->opts->ipaddr && ddos->mode != CGPSDDOS_MASTER) {
		die("the target host option (-t) is only valid in master mode");
	}
	if(ddos->opts->data && ddos->mode == CGPSDDOS_SLAVE) {
		die("the indata option (-i) is only valid in master or local mode");
	}
	if(ddos->opts->port && ddos->mode == CGPSDDOS_LOCAL) {
		die("the port option (-p) is not valid in local mode");
	}
	if(ddos->opts->cgps->result && ddos->mode == CGPSDDOS_SLAVE) {
		die("the result option (-r) is only valid in master or local mode");
	}
	if(ddos->opts->unaddr && ddos->mode != CGPSDDOS_LOCAL) {
		die("the socket option (-s) is only valid in local mode");
	}
	if(ddos->family && ddos->mode == CGPSDDOS_LOCAL) {
		die("the address family option (-4 or -6) is not valid in local mode");
	}
	if(ddos->mode == CGPSDDOS_MASTER) {
		if(!ddos->opts->data) {
			die("input data option (-i) is missing, see --help");
		}
		if(!ddos->opts->ipaddr) {
			die("target host option (-t) is missing, see --help");
		}
		if(!ddos->opts->cgps->result) {
			die("result set option (-r) is missing, see --help");
		}
	}
	if(ddos->mode == CGPSDDOS_SLAVE) {
		if(!ddos->accept) {
			logwarn("no master host defined (-a), see --help");
			logwarn("will accept connections from any host!");
		}
	}
	if(ddos->mode == CGPSDDOS_LOCAL) {
		if(!ddos->opts->unaddr) {
			die("socket path option (-u) is missing, see --help");
		}
		if(!ddos->opts->cgps->result) {
			die("result set option (-r) is missing, see --help");
		}
	}
	
	if(ddos->opts->ipaddr && ddos->opts->unaddr) {
		die("both TCP and UNIX connection requested");
	}
	if(ddos->opts->ipaddr && !ddos->opts->port) {
		ddos->opts->port = CGPSD_DEFAULT_PORT;
	}
	if(ddos->opts->unaddr) {
		struct stat st;
#ifdef HAVE_STAT_EMPTY_STRING_BUG
		if(strlen(ddos->opts->unaddr) == 0) {
			die("path of UNIX socket is empty");
		}
#endif
		if(stat(ddos->opts->unaddr, &st) < 0) {
			die("failed stat UNIX socket (%s)", ddos->opts->unaddr);
		}
	}
	if(!ddos->opts->cgps->format) {
		ddos->opts->cgps->format = CGPS_OUTPUT_FORMAT_DEFAULT;
	}
	if(!ddos->timeout) {
		ddos->timeout = CGPSDDOS_PEER_TIMEOUT;
	}
	if(!ddos->opts->count) {
		ddos->opts->count = 1;
	}
		
	/*
	 * Dump options for debugging purpose.
	 */
	if(ddos->opts->debug) {
		debug("---------------------------------------------");
		debug("options:");
		if(ddos->opts->unaddr) {
			debug("  connect to unix socket = %s", ddos->opts->unaddr);
		}
		if(ddos->opts->ipaddr) {
			char *ipaddr, *host, *port;
			ipaddr = split_hostaddr(strdup(ddos->opts->ipaddr), &host, &port);
			if(port) {
				debug("  target is %s on port %s", host, port);
			} else {
				debug("  target host is %s", host);
			}
			free(ipaddr);
		}
		if(ddos->opts->port) {
			debug("  listen on port %d", ddos->opts->port);
		}
		if(ddos->accept) {
			debug("  accepting connections from %s", ddos->accept);
		}
		if(ddos->slaves) {
			debug("  reading slaves from file %s", ddos->slaves);
		}
		if(ddos->opts->cgps->format) {
			debug("  requested %s as output format", 
			      ddos->opts->cgps->format == CGPS_OUTPUT_FORMAT_PLAIN ? "plain" : "xml");
		}
		if(ddos->opts->data) {
			debug("  reading prediction data from %s", ddos->opts->data);
		}
		if(ddos->timeout) {
			debug("  timeout waiting for peer %ds", ddos->timeout);
		}
		debug("  flags: debug = %s, verbose = %s", 
		      (ddos->opts->debug   ? "yes" : "no"), 
		      (ddos->opts->verbose ? "yes" : "no"));
		debug("---------------------------------------------");
	}
}
