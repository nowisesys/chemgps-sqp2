/* Simca-QP predictions for the ChemGPS project.
 *
 * Copyright (C) 2007-2008 Anders L�vgren and the Computing Department,
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
 *  Contact: Anders L�vgren <anders.lovgren@bmc.uu.se>
 * ----------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>

#include "cgpssqp.h"
#include "cgpsd.h"

/*
 * Initilize server socket.
 */
int init_socket(struct options *opts)
{
	if(opts->ipaddr) {
		struct sockaddr_in sockaddr;
		
		opts->ipsock = socket(PF_INET, SOCK_STREAM, 0);
		if(opts->ipsock < 0) {
			die("failed create TCP socket");
		}
		debug("created TCP socket");
		
		memset(&sockaddr, 0, sizeof(struct sockaddr_in));
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_port = htons(opts->port);
		if(inet_pton(AF_INET, opts->ipaddr, &sockaddr.sin_addr) < 0) {
			die("failed get network address of %s", opts->ipaddr);
		}
		if(bind(opts->ipsock, 
			(const struct sockaddr *)&sockaddr, 
			sizeof(struct sockaddr_in)) < 0) {
			die("failed bind TCP socket");
		}
		debug("bind TCP socket to %s port %d", opts->ipaddr, opts->port);
		
		if(listen(opts->ipsock, opts->backlog) < 0) {
			die("failed listen on TCP socket");
		}
		debug("successful listen on TCP socket (backlog: %d)", opts->backlog);
	}
	if(opts->unaddr) {
		struct sockaddr_un sockaddr;
		
		opts->unsock = socket(PF_UNIX, SOCK_STREAM, 0);
		if(opts->unsock < 0) {
			die("failed create UNIX socket");
		}
		debug("created UNIX socket");
		
		memset(&sockaddr, 0, sizeof(struct sockaddr_un));
		sockaddr.sun_family = AF_UNIX;
		strcpy(sockaddr.sun_path, opts->unaddr);
		if(bind(opts->unsock, (const struct sockaddr *)&sockaddr,
			sizeof(struct sockaddr_un)) < 0) {
			die("failed bind UNIX socket");
		}
		debug("bind UNIX socket to %s", opts->unaddr);
		
		if(listen(opts->unsock, opts->backlog) < 0) {
			die("failed listen on UNIX socket");
		}
		debug("successful listen on UNIX socket (backlog: %d)", opts->backlog);
	}
	return 0;
}