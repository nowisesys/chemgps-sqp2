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
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include "cgpssqp.h"
#include "cgpsclt.h"

static void cleanup_request(struct client *peer, char *buff)
{
	fclose(peer->ss);
	debug("closed socket stream");
	
	if(buff) {
		free(buff);
	}
}

/*
 * Send data from file.
 */
static int request_send_file(const char *file, struct client *peer)
{	
	FILE *fs;
	int c, lines = 0;
	
	fs = fopen(opts->data, "r");
	if(!fs) {
		return -1;
	}
	while((c = getc(fs)) != EOF) {
		if(c == '\n') {
			++lines;
		}
	}
	rewind(fs);
	
	debug("sending data from file %s (lines=%d)", file, lines);	
	
	fprintf(peer->ss, "Load: %d\n", lines);
	while((c = getc(fs)) != EOF) {
		fprintf(peer->ss, "%c", c);
	}
	fprintf(peer->ss, "\n");
	fflush(peer->ss);
	
	fclose(fs);
	
	return 0;
}

/*
 * Send data from stdin.
 */
static int request_send_stdin(struct client *peer)
{
	FILE *out;
	char *inb = NULL, *outb = NULL;
	size_t insize = 0, outsize = 0;
	int lines = 0;
	
	loginfo("waiting for raw data input on stdin (ctrl+d to send)");
	
	out = open_memstream(&outb, &outsize);
	if(!out) {
		return -1;
	}
	while((getline(&inb, &insize, stdin)) != -1) {
		fprintf(out, "%s", inb);
		++lines;
	}
	fclose(out);
	
	debug("sending data from stdin (lines=%d)", lines);	
	
	fprintf(peer->ss, "Load: %d\n", lines);
	fprintf(peer->ss, "%s", outb);
	fflush(peer->ss);
	
	free(outb);
	free(inb);
	
	return 0;
}

void request(struct options *opts, struct client *peer)
{
	const struct cgps_result_entry *entry;
        struct request_option req;
	char *buff = NULL;
	size_t size = 0;
	int delim = 0, done = 0;
	int c;
	
	peer->ss = fdopen(dup(peer->sock), "r+");
	if(!peer->ss) {
		cleanup_request(peer, buff);
		die("failed open socket stream");
	}
	debug("opened socket stream");

	debug("reading greeting");
	read_request(&buff, &size, peer->ss);
	debug("received: '%s'", buff);
	
	debug("sending greeting");
	fprintf(peer->ss, "CGPSP %s (%s: client ready)\n", CGPSP_PROTO_VERSION, opts->prog);
	fflush(peer->ss);
	
	debug("sending prediction request");
	fprintf(peer->ss, "Predict: ");
	for(entry = cgps_result_entry_list; entry->name; ++entry) {	
		if(cgps_result_isset(opts->cgps->result, entry->value)) {
			if(delim++) {
				fprintf(peer->ss, ":");
			}
			fprintf(peer->ss, "%s", entry->name);
		}
	}
	fprintf(peer->ss, "\n");
	fflush(peer->ss);

	debug("sending format request");
	if(opts->cgps->format == CGPS_OUTPUT_FORMAT_PLAIN) {
		fprintf(peer->ss, "Format: plain\n");
	} else {
		fprintf(peer->ss, "Format: xml\n");
	}
	fflush(peer->ss);
	
	while(!done) {
		debug("waiting for server request");
		read_request(&buff, &size, peer->ss);
		debug("received: '%s'", buff);
	        if(split_request_option(buff, &req) < 0) {
			cleanup_request(peer, buff);
			die("protocol error (%s unexpected)", req.option);
		}
		
		switch(req.symbol) {
		case CGPSP_PROTO_LOAD:
			debug("received load request");
			if(opts->data) {
				request_send_file(opts->data, peer);
			} else {
				request_send_stdin(peer);
			}
			break;
		case CGPSP_PROTO_RESULT:
			debug("received result request");
			while((c = getc(peer->ss)) != -1) {
				printf("%c", c);
			}				
			done = 1;
			break;
		default:
			die("protocol error (%s unexpected)", req.option);
			cleanup_request(peer, buff);
		}
	}
	debug("done with request");
	
	cleanup_request(peer, buff);
}
