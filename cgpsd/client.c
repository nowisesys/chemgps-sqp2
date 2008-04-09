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
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include "cgpssqp.h"
#include "cgpsd.h"

static int cleanup_request(struct client *peer, char *buff, int status)
{	
	if(peer->ss) {
		fclose(peer->ss);
		debug("closed socket stream");
	}
	close(peer->sock);
	debug("closed peer socket");
	
	if(buff) {
		free(buff);
	}
	free(peer);	
	
	return status;
}

/*
 * Process a peer request. The params argument should have been allocated on
 * the heap and point to a client struct. The void * argument is required as
 * this function might be the start routine of a thread.
 */
int process_request(void *params)
{
	struct client *peer = (struct client *)params;
	char *buff = NULL;
	size_t size = 0;
	struct request_option req;
	struct cgps_options cgps;
	struct cgps_project proj;
	struct cgps_predict pred;
	struct cgps_result res;
	int model, i;
	
	peer->ss = fdopen(dup(peer->sock), "r+");
	if(!peer->ss) {
		logerr("failed open socket stream");
		return cleanup_request(peer, buff, -1);
	}
	debug("opened socket stream");

	debug("sending greeting");
	fprintf(peer->ss, "CGPSP %s (%s: server ready)\n", CGPSP_PROTO_VERSION, opts->prog);
	fflush(peer->ss);
	
	debug("reading greeting");
	read_request(&buff, &size, peer->ss);
	debug("received: '%s'", buff);

	debug("copying global libchemgps options");
	cgps = *opts->cgps;
	debug("copying project");
	proj = *peer->proj;
	proj.opts = &cgps;

	debug("receiving predict request");
	read_request(&buff, &size, peer->ss);
	debug("received: '%s'", buff);
	if(split_request_option(buff, &req) < 0) {
		logerr("failed read client option (%s)", buff);
		return cleanup_request(peer, buff, -1);
	}
	if(req.symbol != CGPSP_PROTO_PREDICT) {
		logerr("protocol error (expected predict option, got %s)", req.option);
		return cleanup_request(peer, buff, -1);
	}
	cgps.result = cgps_get_predict_mask(req.value);

	debug("receiving format request");
	read_request(&buff, &size, peer->ss);
	debug("received: '%s'", buff);
	if(split_request_option(buff, &req) < 0) {
		logerr("failed read client option (%s)", buff);
		return cleanup_request(peer, buff, -1);
	}
	if(req.symbol != CGPSP_PROTO_FORMAT) {
		logerr("protocol error (expected format option, got %s)", req.option);
		return cleanup_request(peer, buff, -1);
	}
	if(strcmp("plain", req.value) == 0) {
		cgps.format = CGPS_OUTPUT_FORMAT_PLAIN;
	} else if(strcmp("xml", req.value) == 0) {
		cgps.format = CGPS_OUTPUT_FORMAT_XML;
	} else {
		logerr("protocol error (invalid format argument, got %s)", req.value);
		return cleanup_request(peer, buff, -1);
	}
	
	for(i = 1; i <= proj.models; ++i) {	
		cgps_predict_init(&proj, &pred, peer);
		debug("initilized for prediction");
		if((model = cgps_predict(&proj, i, &pred)) != -1) {
			debug("predict called (index=%d, model=%d)", i, model);
			if(cgps_result_init(&proj, &res) == 0) {
				debug("intilized prediction result");
				fprintf(peer->ss, "Result:\n");
				fflush(peer->ss);
				if(cgps_result(&proj, model, &pred, &res, peer->ss) == 0) {
					debug("successful got result");
				}
				fflush(peer->ss);
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
	
	return cleanup_request(peer, buff, 0);
}
