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
	int sockets = 0;
	
	if(opts->ipaddr) {
		loginfo("listening on TCP socket %s port %d", opts->ipaddr, opts->port);
	}
	if(opts->unaddr) {
		loginfo("listening on UNIX socket %s", opts->unaddr);
	}
	
	FD_ZERO(&sockfds);
	if(opts->ipsock) {
		debug("adding the TCP server socket to socket file descriptors set");
		FD_SET(opts->ipsock, &sockfds);
		sockets++;
	}
	if(opts->unsock) {
		debug("adding the UNIX server socket to socket file descriptors set");
		FD_SET(opts->unsock, &sockfds);
		sockets++;
	}
	while(!opts->done) {
		fd_set ready = sockfds;
		int result, i;
		
		result = select(sockets, &ready, NULL, NULL, NULL);
		if(result < 0) {
			logerr("failed select");
		} else if(!result) {
			debug("timeout in select");
		} else {
			debug("select has returned");
			
			for(i = 0; i < result; ++i) {
				if(FD_ISSET(i, &ready)) {
					int client = -1;

					debug("file descriptor %d is set", i);
					if(i == opts->ipsock) {
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
					} else if(i == opts->unsock) {
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
								
								debug("UNIX socket peer: pid = %d, uid = %d, gid = %d\n",
								      cred.pid, cred.uid, cred.gid);
								loginfo("accepted UNIX client connection from %s (uid: %d, pid: %d)", 
									pwent->pw_name, cred.uid, cred.pid);
							}
						}
					} else {
						logerr("unexpected socket %d in ready set", i);
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
		}
	}
	debug("the done flag is set, exiting service()");
}
