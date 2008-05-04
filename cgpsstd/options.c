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
	printf("Copyright (C) 2007-2008 by Anders Lövgren and the Computing Department at\n");
	printf("Uppsala Biomedical Centre (BMC), Uppsala University.\n");	
}

void parse_options(int argc, char **argv, struct options *opts)
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
	
	while((c = getopt_long(argc, argv, "bdf:h:i:l:n:o:p:r:svV", options, &index)) != -1) {
		switch(c) {
		case 'b':
			opts->batch = 1;
			break;
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
		case 'i':
			opts->data = malloc(strlen(optarg) + 1);
			if(!opts->data) {
				die("failed alloc memory");
			}
			strcpy(opts->data, optarg);
			break;
		case 'l':
			opts->cgps->logfile = malloc(strlen(optarg) + 1);
			if(!opts->cgps->logfile) {
				die("failed alloc memory");
			}
			strcpy(opts->cgps->logfile, optarg);
			break;
		case 'n':
			opts->numobs = atoi(optarg);
			break;
		case 'o':
			opts->output = malloc(strlen(optarg) + 1);
			if(!opts->output) {
				die("failed alloc memory");
			}
			strcpy(opts->output, optarg);
			break;
		case 'p':
			opts->proj = malloc(strlen(optarg) + 1);
			if(!opts->proj) {
				die("failed alloc memory");
			}
			strcpy(opts->proj, optarg);
			break;
		case 'r':
			opts->cgps->result = cgps_get_predict_mask(optarg);
			break;
		case 's':
			debug("enabling syslog (bye-bye console ;-))");
			opts->syslog = 1;
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
	 * Check that required arguments where given.
	 */
	if(!opts->proj) {
		die("project file option (-p) is missing");
	}
	if(!opts->cgps->format) {
		opts->cgps->format = CGPS_OUTPUT_FORMAT_DEFAULT;
	}
	
	/* 
	 * If we where called in a pipe-line and input data is empty, 
	 * then set raw data input to be stdin.
	 */
	if(!isatty(0)) {
		if(opts->data && !opts->batch) {
			logwarn("called in pipe with input file enabled (not reading raw data input from stdin)");
		}
		/*
		 * Enable batch mode if called in pipe.
		 */
		opts->batch = 1;
	}
	
	/*
	 * Check that input files exists.
	 */
	if(stat(opts->proj, &st) < 0) {
		die("failed stat project file (model) %s", opts->proj);
	}	
	if(opts->data) {
		if(stat(opts->data, &st) < 0) {
			die("failed stat raw input data file %s", opts->data);
		}
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
	
	/*
	 * Dump options for debugging purpose.
	 */
	if(opts->debug) {
		debug("---------------------------------------------");
		debug("options:");
		debug("  project file path (model) = %s", opts->proj);
		if(opts->data) {
			debug("  input data file (raw) = %s", opts->data);
		} else {
			debug("  input data (raw) is read from stdin");
		}
		if(opts->output) {
			debug("  output file (result) = %s", opts->output);
		} else {
			debug("  output (result) is written to stdout");
		}
		if(opts->cgps->logfile) {
			debug("  simca lib logfile = %s", opts->cgps->logfile);
		}
		debug("  flags: debug = %s, use syslog = %s, verbose = %s", 
		      (opts->debug   ? "yes" : "no"), 
		      (opts->syslog  ? "yes" : "no"),
		      (opts->verbose ? "yes" : "no"));
		debug("  libchemgps: format = %d, result = %d",
		      opts->cgps->format, opts->cgps->result);
		debug("---------------------------------------------");
	}
}
