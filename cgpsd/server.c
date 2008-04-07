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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <pwd.h>

#include "cgpssqp.h"
#include "cgpsd.h"

/*
 * Start accepting connections on server socket(s).
 */
void service(struct options *opts)
{
	fd_set sockfds;  /* server socket file descriptors set */
	int maxsock = 0;
	
	if(opts->ipaddr) {
		loginfo("listening on TCP socket %s port %d", opts->ipaddr, opts->port);
	}
	if(opts->unaddr) {
		loginfo("listening on UNIX socket %s", opts->unaddr);
	}
	
	FD_ZERO(&sockfds);
	if(opts->ipsock) {
		debug("adding TCP server socket to read ready set (fd = %d)", opts->ipsock);
		FD_SET(opts->ipsock, &sockfds);
		maxsock = opts->ipsock > maxsock ? opts->ipsock : maxsock;
	}
	if(opts->unsock) {
		debug("adding UNIX server socket to read ready set (fd = %d)", opts->unsock);
		FD_SET(opts->unsock, &sockfds);
		maxsock = opts->unsock > maxsock ? opts->unsock : maxsock;
	}

        setup_signals(opts);
	
	debug("enter running state");
        opts->state |= CGPSD_STATE_RUNNING;
	
	while(1) {
		fd_set readfds = sockfds;
		int result, client = -1;

		debug("waiting for client connections...");
		result = select(maxsock + 1, &readfds, NULL, NULL, NULL);
		
		if(cgpsd_done(opts->state)) {
			break;
		}
		
		if(result < 0) {
			logerr("failed select");
		} else {
			debug("select returned with result = %d");
			if(FD_ISSET(opts->ipsock, &readfds)) {
				struct sockaddr_in sockaddr;
				socklen_t socklen = sizeof(struct sockaddr_in);
				client = accept(opts->ipsock,
						(struct sockaddr *)&sockaddr,
						&socklen);
				if(client < 0) {
					logerr("failed accept TCP client connection");
				} else {
					loginfo("accepted TCP client connection from %s on port %d", 
						inet_ntoa(sockaddr.sin_addr), 
						ntohs(sockaddr.sin_port));
				}
			}
			if(FD_ISSET(opts->unsock, &readfds)) {
				struct sockaddr_un sockaddr;
				socklen_t socklen = sizeof(struct sockaddr_un);
				client = accept(opts->unsock,
						(struct sockaddr *)&sockaddr,
						&socklen);
				if(client < 0) {
					logerr("failed accept UNIX client connection");
				} else {
					struct ucred cred;
					int credlen = sizeof(struct ucred);
					
					if(getsockopt(client, SOL_SOCKET, SO_PEERCRED, &cred, &credlen) < 0) {
						logerr("failed get credentials of UNIX socket peer");
					} else {
						struct passwd *pwent = getpwuid(cred.uid);
						
						debug("UNIX socket peer: pid = %d, uid = %d, gid = %d",
						      cred.pid, cred.uid, cred.gid);
						loginfo("accepted UNIX client connection from %s (uid: %d, pid: %d)", 
							pwent->pw_name, cred.uid, cred.pid);
					}
				}
			}
			
			if(client != -1) {
				struct client *peer = malloc(sizeof(struct client));
				if(!peer) {
					logerr("failed alloc memory");
					continue;
				}
				
				peer->opts = opts;
				peer->sock = client;
				
				process_peer_request(peer);
			}
		}
	}
	debug("the done flag is set, exiting service()");

        restore_signals(opts);
}
