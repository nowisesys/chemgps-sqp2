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
#ifdef HAVE_STRING_H
# include <string.h>
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
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include <ctype.h>

#include "cgpssqp.h"
#include "cgpsclt.h"

static void cleanup_request(struct client *peer, char *buff, FILE *outs)
{
	if(peer) {
		if(peer->ss) {
			fclose(peer->ss);
			debug("closed socket stream");
		}
	}
	if(buff) {
		free(buff);
	}
	if(outs != stdout) {
		fclose(outs);
	}
}

/*
 * Send data from file.
 */
static int request_send_file(const char *file, struct client *peer)
{	
	FILE *fs;
	int c, lines = 0, header = 0;
	
	fs = fopen(file, "r");
	if(!fs) {
		return -1;
	}
	
	c = getc(fs);
	if(!isdigit(c) && c != '-') {
		debug("header detected in data file");
		header = 1;
	}
	ungetc(c, fs);
	
	while((c = getc(fs)) != EOF) {
		if(c == '\n') {
			++lines;
		}
	}
	rewind(fs);
	
	debug("sending data from file %s (lines=%d)", file, lines - header);	
	
	fprintf(peer->ss, "Load: %d\n", lines - header);
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
	int lines = 0, header = 0;
	
	loginfo("waiting for raw data input on stdin (ctrl+d to send)");
	
	out = open_memstream(&outb, &outsize);
	if(!out) {
		return -1;
	}
	while((getline(&inb, &insize, stdin)) != -1) {
		fprintf(out, "%s", inb);
		if(!header && !isdigit(inb[0]) && inb[0] != '-') {
			debug("header detected in data file");
			header = 1;
		}
		++lines;
	}
	fclose(out);
	
	debug("sending data from stdin (lines=%d)", lines - header);	
	
	fprintf(peer->ss, "Load: %d\n", lines - header);
	fprintf(peer->ss, "%s\n", outb);
	fflush(peer->ss);
	
	free(outb);
	free(inb);
	
	return 0;
}

/*
 * Send data from memory buffer.
 */
static int request_send_buffer(const char *buffer, struct client *peer)
{
	int lines = 0, header = 0;
	const char *curr = buffer;
	
	while(*curr) {
		if(!header && !isdigit(*curr) && *curr != '-') {
			debug("header detected in data file");
			header = 1;
		}		
		if(*curr++ == '\n') ++lines;
	}
	debug("sending data from memory buffer (lines=%d)", lines - header);	
	
	fprintf(peer->ss, "Load: %d\n", lines - header);
	fprintf(peer->ss, "%s\n", buffer);
	fflush(peer->ss);
	
	return 0;
}

int request(struct options *popt, struct client *peer)
{
	const struct cgps_result_entry *entry;
        struct request_option req;
	struct stat st;
	FILE *fsout = stdout;
	char *buff = NULL;
	size_t size = 0;
	int delim = 0, done = 0;
	int c;
	
	peer->ss = fdopen(dup(peer->sock), "r+");
	if(!peer->ss) {
		if(errno == EBADF) {
			cleanup_request(peer, buff, fsout);
			return CGPSCLT_CONN_RETRY;
		} 
		cleanup_request(peer, buff, fsout);
		logerr("failed open socket stream");
		return CGPSCLT_CONN_FAILED;
	}
	errno = 0;
	debug("opened socket stream");
	
	debug("reading greeting");
	if(read_request(&buff, &size, peer->ss) < 0) {
		cleanup_request(peer, buff, fsout);
		return CGPSCLT_CONN_RETRY;
	}
	debug("received: '%s'", buff);

	debug("sending greeting");
	if(fprintf(peer->ss, "CGPSP %s (%s: client ready)\n", CGPSP_PROTO_VERSION, popt->prog) > 0) {
		fflush(peer->ss);
	}
	if(errno == EPIPE) {
		cleanup_request(peer, buff, fsout);
		return CGPSCLT_CONN_RETRY;
	}
	
	debug("sending prediction request");
	fprintf(peer->ss, "Predict: ");
	if(errno == EPIPE) {
		cleanup_request(peer, buff, fsout);
		return CGPSCLT_CONN_RETRY;
	}
	for(entry = cgps_result_entry_list; entry->name; ++entry) {	
		if(cgps_result_isset(popt->cgps->result, entry->value)) {
			if(delim++) {
				fprintf(peer->ss, ":");
			}
			fprintf(peer->ss, "%s", entry->name);
		}
		if(errno == EPIPE) {
			cleanup_request(peer, buff, fsout);
			return CGPSCLT_CONN_RETRY;
		}
	}
	if(fprintf(peer->ss, "\n") > 0) {
		fflush(peer->ss);
	}
	if(errno == EPIPE) {
		cleanup_request(peer, buff, fsout);
		return CGPSCLT_CONN_RETRY;
	}

	debug("sending format request");
	if(popt->cgps->format == CGPS_OUTPUT_FORMAT_PLAIN) {
		fprintf(peer->ss, "Format: plain\n");
	} else {
		fprintf(peer->ss, "Format: xml\n");
	}
	fflush(peer->ss);
	if(errno == EPIPE) {
		cleanup_request(peer, buff, fsout);
		return CGPSCLT_CONN_RETRY;
	}

	if(popt->output) {
		fsout = fopen(popt->output, "w");
		if(!fsout) {
			cleanup_request(peer, buff, fsout);
			logerr("failed open output file %s", popt->output);
			return CGPSCLT_CONN_FAILED;
		}
	}
	
	while(!done) {
		debug("waiting for server request");
		if(read_request(&buff, &size, peer->ss) < 0) {
			cleanup_request(peer, buff, fsout);
			return CGPSCLT_CONN_RETRY;
		}

		debug("received: '%s'", buff);
	        if(split_request_option(buff, &req) == CGPSP_PROTO_LAST) {
			logerr("protocol error (%s unexpected)", req.option);
			cleanup_request(peer, buff, fsout);
			return CGPSCLT_CONN_FAILED;
		}
		
		switch(req.symbol) {
		case CGPSP_PROTO_LOAD:
			debug("received load request");
			if(popt->data) {
				if(stat(popt->data, &st) == 0) {
					request_send_file(popt->data, peer);
				} else {
					request_send_buffer(popt->data, peer);
				}
			} else {
				request_send_stdin(peer);
			}
			break;
		case CGPSP_PROTO_RESULT:
			debug("received result request");
			while((c = getc(peer->ss)) != EOF) {
				if(!popt->quiet) {
					fprintf(fsout, "%c", c);
				}
			}
			done = 1;
			break;
		case CGPSP_PROTO_ERROR:
			logerr("server response: %s", req.value);
			cleanup_request(peer, buff, fsout);
			return CGPSCLT_CONN_FAILED;
		default:
			logerr("protocol error (%s unexpected)", req.option);
			cleanup_request(peer, buff, fsout);
			return CGPSCLT_CONN_FAILED;
		}
	}
	debug("done with request");
	
	cleanup_request(peer, buff, fsout);
	return CGPSCLT_CONN_SUCCESS;
}
