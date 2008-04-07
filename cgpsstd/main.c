/* Make Simca-QP predictions for the ChemGPS project.
 * 
 * Copyright (C) 2007-2008 Computing Department at BMC, Uppsala Biomedical
 * Centre, Uppsala University.
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
 *  Contact: Anders L�vgren <anders.lovgren@bmc.uu.se>
 * ----------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <chemgps.h>

#include "cgpssqp.h"

struct options *opts = NULL;

void parse_options(int argc, char **argv, struct options *opts);

void exit_handler(void)
{
	if(opts) {
		debug("cleaning up at exit...");
		
		if(opts->cgps) {
			if(opts->cgps->logfile) {
				free(opts->cgps->logfile);
				opts->cgps->logfile = NULL;
			}
			free(opts->cgps);
			opts->cgps = NULL;
		}		
		if(opts->data) {
			free(opts->data);
			opts->data = NULL;
		}
		if(opts->output) {
			free(opts->output);
			opts->output = NULL;
		}
		if(opts->proj) {
			free(opts->proj);
			opts->proj = NULL;
		}
		free(opts);
		opts = NULL;
	}
}

void make_prediction(struct options *opts)
{	
	struct cgps_project proj;
	struct cgps_predict pred;
	struct cgps_result res;
	int i, model;

	opts->cgps->logger = cgps_syslog;
	opts->cgps->indata = cgps_predict_data;
	
	if(opts->debug > 1) {
		debug("enable library debug");
		opts->cgps->debug = opts->debug;
		opts->cgps->verbose = opts->verbose;
	}
	opts->cgps->syslog = opts->syslog;
	
	if(cgps_project_load(&proj, opts->proj, opts->cgps) == 0) {			
		debug("successful loaded project %s", opts->proj);
		debug("project got %d models", proj.models);
		for(i = 1; i <= proj.models; ++i) {			
			cgps_predict_init(&proj, &pred);
			debug("initilized for prediction");
			if((model = cgps_predict(&proj, i, &pred)) != -1) {					
				debug("predict called (index=%d, model=%d)", i, model);
				if(cgps_result_init(&proj, &res) == 0) {
					debug("intilized prediction result");
					if(cgps_result(&proj, model, &pred, &res, stdout) == 0) {
						debug("successful got result");
					}
					debug("cleaning up the result");
					cgps_result_cleanup(&proj, &res);
				}
			}
			else {
				logerr("failed predict");
			}
			debug("cleaning up after predict");
			cgps_predict_cleanup(&proj, &pred);
		}
		debug("closing project");
		cgps_project_close(&proj);
	}
	else {
		die("failed load project %s", opts->proj);
	}
}

int main(int argc, char **argv)
{	
        opts = malloc(sizeof(struct options));
	if(!opts) {
		die("failed alloc memory");
	}
	memset(opts, 0, sizeof(struct options));
	
	opts->cgps = malloc(sizeof(struct cgps_options));
	if(!opts->cgps) {
		die("failed alloc memory");
	}
	memset(opts->cgps, 0, sizeof(struct cgps_options));
	
	opts->prog = basename(argv[0]);
	opts->parent = getpid();
	
	if(atexit(exit_handler) != 0) {
		logerr("failed register main exit handler");
	}
	parse_options(argc, argv, opts);	
	make_prediction(opts);
			
	return 0;
}