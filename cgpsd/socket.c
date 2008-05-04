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

#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#include <sys/un.h>

#include "cgpssqp.h"
#include "cgpsd.h"

/*
 * Initilize server socket.
 */
int init_socket(struct options *opts)
{
	if(opts->ipaddr) {
		struct addrinfo hints, *addr, *next = NULL;
                char port[6];
                char host[NI_MAXHOST], serv[NI_MAXSERV];
                int res;
		
                snprintf(port, sizeof(port) - 1, "%d", opts->port);
		
		hints.ai_family = opts->family;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_ADDRCONFIG;
		hints.ai_protocol = 0;
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;
		hints.ai_next = NULL;
		
		if(opts->ipaddr == CGPSD_DEFAULT_ADDR) {
			hints.ai_flags |= AI_PASSIVE;
			res = getaddrinfo(NULL, port, &hints, &addr);
		} else {
			res = getaddrinfo(opts->ipaddr, port, &hints, &addr);
		}
		if(res != 0) {
			die("failed resolve %s:%d (%s)",
			    opts->ipaddr, opts->port, gai_strerror(res));
		}
		
		for(next = addr; next != NULL; next = next->ai_next) {
			opts->ipsock = socket(next->ai_family, next->ai_socktype, next->ai_protocol);
			if(opts->ipsock < 0) {
				continue;
			}
			res = bind(opts->ipsock, next->ai_addr, next->ai_addrlen);
			if(res < 0) {
				close(opts->ipsock);
				continue;
			}
			break;
		}
		if(!next) {
			freeaddrinfo(addr);
			if(opts->ipsock < 0) {
				die("failed create TCP socket");
			}
			if(res < 0) {
				die("failed bind to %s:%d", opts->ipaddr, opts->port);
			}
		}
		
		if(opts->debug) {
			if(getnameinfo(next->ai_addr,
				       next->ai_addrlen,
				       host, NI_MAXHOST,
				       serv, NI_MAXSERV, NI_NUMERICSERV) == 0) {
				debug("bind TCP socket to %s port %s (%s)",
				      host, serv, next->ai_family == AF_INET ? "ipv4" : "ipv6");
			} else {
				debug("bind TCP socket to %s port %d (%s)",
				      opts->ipaddr, opts->port,
				      next->ai_family == AF_INET ? "ipv4" : "ipv6");
			}
		}
		
		/* debug("bind TCP socket to %s port %d (%s)",  */
		/*       opts->ipaddr, opts->port, next->ai_family == AF_INET ? "ipv4" : "ipv6"); */
		
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
		strncpy(sockaddr.sun_path, opts->unaddr, sizeof(sockaddr.sun_path));
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

/*
 * Close server socket(s).
 */
void close_socket(struct options *opts)
{
	if(opts->ipsock && opts->ipsock != -1) {
		if(shutdown(opts->ipsock, SHUT_RDWR) < 0) {
			logerr("failed close TCP socket");
		} else {
			debug("closed TCP socket");
		}
	}
	if(opts->unsock && opts->unsock != -1) {
		if(shutdown(opts->unsock, SHUT_RDWR) < 0) {
			logerr("failed close UNIX socket");
		} else {
			debug("closed UNIX socket");
		}
	}
}
