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

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#include "cgpsddos.h"
#include "cgpssqp.h"

/*
 * Create a named TCP or UDP socket (family). The family is either AF_INET or AF_INET6
 * or AF_UNSPEC (any IPv4 or IPv6). The type argument is either SOCK_STREAM or SOCK_DGRAM.
 * See getaddrinfo(3) for info on the addr, port and flags argument.
 */
int open_named_socket(int family, int type, const char *addr, const char *port, int flags)
{
	struct addrinfo hints, *res, *next;
	int sock;
	
	hints.ai_family = family;
	hints.ai_socktype = type;
	hints.ai_flags = flags;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	
	if(getaddrinfo(addr, port, &hints, &res) != 0) {
		logerr("failed lookup address %s and service %s", addr, port);
		return -1;
	}
	for(next = res; next != NULL; next = next->ai_next) {
		sock = socket(next->ai_family, next->ai_socktype, next->ai_protocol);
		if(sock < 0) {
			continue;
		}
		if(bind(sock, next->ai_addr, next->ai_addrlen) == 0) {
			struct protoent *protocol = getprotobynumber(next->ai_protocol);
			if(!protocol) {
				logerr("failed lookup protocol %d", next->ai_protocol);
			} else {
				char host[NI_MAXHOST];
				char service[NI_MAXSERV];
				if(getnameinfo(next->ai_addr,
					       next->ai_addrlen, 
					       host, NI_MAXHOST,
					       service, NI_MAXSERV, NI_NUMERICSERV) == 0) {
					debug("bind to %s on port %s using protocol %s (%s)", 
					      host, service, protocol->p_name,
					      next->ai_family == AF_INET ? "ipv4" : "ipv6");
				}
			}
			break;
		}
	}
	if(!next) {
		freeaddrinfo(res);
		return -1;
	}
	   
	freeaddrinfo(res);
	return sock;
}
