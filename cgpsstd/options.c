/* Simca-QP predictions for the ChemGPS project.
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
#include <getopt.h>
#include <errno.h>
#include <libgen.h>
#include <chemgps.h>

#include "cgpssqp.h"

static void usage(const char *prog, const char *section)
{
	if(!section) {
		printf("%s - standalone prediction program using libchemgpg and Simca-QP.\n", prog);
		printf("\n");
		printf("Usage: %s -p proj -r data [options...]\n", prog);
		printf("Options:\n");
		printf("  -p, --proj=path:    Load project file\n");
		printf("  -i, --data=path:    Raw data input file (default=stdin)\n");
		printf("  -o, --output=path:  Write result to output file (default=stdout)\n");
		printf("  -l, --logfile=path: Use path as simca lib log\n");
		printf("  -r, --result=str:   Colon separated list of results to show (see -h result)\n");
		printf("  -n, --numobs=num:   Number of observations in input data (see -i option) (default=%d)\n", DEFAULT_NUMBER_OBSERVATIONS);
		printf("  -s, --syslog:       Use syslog(3) for application logging\n");
		printf("  -f, --format=str:   Set ouput format (either plain or xml)\n");
		printf("  -b, --batch:        Enable batch job mode (suppress some messages)\n");
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
	printf("A standalone prediction application using libchemgps and Umetrics Simca-QP.\n");
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
		{ "proj",    1, 0, 'p' },
		{ "data",    1, 0, 'i' },
		{ "output",  1, 0, 'o' },
		{ "logfile", 1, 0, 'l' },
		{ "result",  1, 0, 'r' },
		{ "numobs",  1, 0, 'n' },
		{ "syslog",  0, 0, 's' },
		{ "format",  1, 0, 'f' }, 
		{ "batch",   0, 0, 'b' },
#if ! defined(NDEBUG)
		{ "debug",   0, 0, 'd' },
#endif
		{ "verbose", 0, 0, 'v' },
		{ "help",    2, 0, 'h' },
		{ "version", 0, 0, 'V' }
	};
	int optindex, c;
	struct stat st;

	if(argc < 3) {
		usage(popt->prog, NULL);
		exit(1);
	}
	
	while((c = getopt_long(argc, argv, "bdf:h:i:l:n:o:p:r:svV", options, &optindex)) != -1) {
		switch(c) {
		case 'b':
			popt->batch = 1;
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
		case 'i':
			popt->data = malloc(strlen(optarg) + 1);
			if(!popt->data) {
				die("failed alloc memory");
			}
			strcpy(popt->data, optarg);
			break;
		case 'l':
			popt->cgps->logfile = malloc(strlen(optarg) + 1);
			if(!popt->cgps->logfile) {
				die("failed alloc memory");
			}
			strcpy(popt->cgps->logfile, optarg);
			break;
		case 'n':
			popt->numobs = atoi(optarg);
			break;
		case 'o':
			popt->output = malloc(strlen(optarg) + 1);
			if(!popt->output) {
				die("failed alloc memory");
			}
			strcpy(popt->output, optarg);
			break;
		case 'p':
			popt->proj = malloc(strlen(optarg) + 1);
			if(!popt->proj) {
				die("failed alloc memory");
			}
			strcpy(popt->proj, optarg);
			break;
		case 'r':
			popt->cgps->result = cgps_get_predict_mask(optarg);
			break;
		case 's':
			debug("enabling syslog (bye-bye console ;-))");
			popt->syslog = 1;
			break;
		case 'v':
			popt->verbose++;
			break;
		case 'V':
			version(popt->prog);
			exit(0);
		}
	}

	/*
	 * Check that required arguments where given.
	 */
	if(!popt->proj) {
		die("project file option (-p) is missing");
	}
	if(!popt->cgps->format) {
		popt->cgps->format = CGPS_OUTPUT_FORMAT_DEFAULT;
	}
	
	/* 
	 * If we where called in a pipe-line and input data is empty, 
	 * then set raw data input to be stdin.
	 */
	if(!isatty(0)) {
		if(popt->data && !popt->batch) {
			logwarn("called in pipe with input file enabled (not reading raw data input from stdin)");
		}
		/*
		 * Enable batch mode if called in pipe.
		 */
		popt->batch = 1;
	}
	
	/*
	 * Check that input files exists.
	 */
	if(stat(popt->proj, &st) < 0) {
		die("failed stat project file (model) %s", popt->proj);
	}	
	if(popt->data) {
		if(stat(popt->data, &st) < 0) {
			die("failed stat raw input data file %s", popt->data);
		}
	}
	if(popt->cgps->logfile) {
		if(stat(popt->cgps->logfile, &st) < 0) {
			char *dir = dirname(popt->cgps->logfile);
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
	
	/*
	 * Dump options for debugging purpose.
	 */
	if(popt->debug) {
		debug("---------------------------------------------");
		debug("options:");
		debug("  project file path (model) = %s", popt->proj);
		if(popt->data) {
			debug("  input data file (raw) = %s", popt->data);
		} else {
			debug("  input data (raw) is read from stdin");
		}
		if(popt->output) {
			debug("  output file (result) = %s", popt->output);
		} else {
			debug("  output (result) is written to stdout");
		}
		if(popt->cgps->logfile) {
			debug("  simca lib logfile = %s", popt->cgps->logfile);
		}
		debug("  flags: debug = %s, use syslog = %s, verbose = %s", 
		      (popt->debug   ? "yes" : "no"), 
		      (popt->syslog  ? "yes" : "no"),
		      (popt->verbose ? "yes" : "no"));
		debug("  libchemgps: format = %d, result = %d",
		      popt->cgps->format, popt->cgps->result);
		debug("---------------------------------------------");
	}
}
