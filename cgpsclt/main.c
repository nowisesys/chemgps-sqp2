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

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <signal.h>
#include <libgen.h>
#include <chemgps.h>

#include "cgpssqp.h"
#include "cgpsclt.h"

struct options *opts;

#ifdef HAVE_ATEXIT
void exit_handler(void)
{
	if(opts) {
		debug("cleaning up at exit...");
		
		if(opts->proj) {
			free(opts->proj);
			opts->proj = NULL;
		}
		if(opts->ipaddr) {
			shutdown(opts->ipsock, SHUT_RDWR);
			if(opts->ipaddr != CGPSD_DEFAULT_ADDR) {
				free(opts->ipaddr);
			}
			opts->ipaddr = NULL;
		}
		if(opts->unaddr) {
			shutdown(opts->unsock, SHUT_RDWR);
			if(opts->unaddr != CGPSD_DEFAULT_SOCK) {
				free(opts->unaddr);
			}
			opts->unaddr = NULL;
		}
		free(opts);
		opts = NULL;
	}
}
#endif /* HAVE_ATEXIT */

int main(int argc, char **argv)
{
	struct client peer;
	int retry = 0;
	
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
	
#ifdef HAVE_ATEXIT
	if(atexit(exit_handler) != 0) {
		logerr("failed register main exit handler");
	}
#endif

	parse_options(argc, argv, opts);
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		die("failed ignore broken pipe siganl (SIGPIPE)");
	}
	while(retry < CGPSCLT_RETRY_LIMIT) {
		if(retry != 0) {
			loginfo("retry attempt %d/%d", retry, CGPSCLT_RETRY_LIMIT);
		}
		retry++;
		
		switch(init_socket(opts)) {
		case CGPSCLT_CONN_FAILED:
			exit(1);
			break;
		case CGPSCLT_CONN_RETRY:
			sleep(CGPSCLT_RETRY_SLEEP);
			continue;
		}
		
		peer.sock = opts->unsock ? opts->unsock : opts->ipsock;
		peer.opts = opts;
	
		switch(request(opts, &peer)) {
		case CGPSCLT_CONN_FAILED:
			exit(1);
			break;
		case CGPSCLT_CONN_RETRY:
			sleep(CGPSCLT_RETRY_SLEEP);
			continue;
		case CGPSCLT_CONN_SUCCESS:
			retry = CGPSCLT_RETRY_LIMIT;
			break;
		}
		break;
	}
	
	return 0;
}
